#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Receive-only Linux bench console. No target command path exists."""
import binascii
from collections import deque
import glob
import json
import os
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

MAX_REPLAY = 1024 * 1024
FIELDS = ('time state levels quiet last_owner switches early overlaps overlap_ms '
          'unrequested gaps tx_fail faults minimum h0 h1 h2 h3 '
          'wait_a wait_b bypass_a bypass_b starved_a starved_b services_a services_b').split()
COUNTERS = ('switches early overlaps overlap_ms unrequested gaps tx_fail h0 h1 h2 h3 '
            'starved_a starved_b services_a services_b').split()

class Model:
    def __init__(self, source):
        self.source = source
        self.rows = deque(maxlen=256)
        self.buffer = b''
        self.errors = 0
        self.policy = False
        self.frozen = ''
        self.received = None
        self.connected = False
        self.reset_flags = None

    def freeze(self, why):
        self.frozen = why

    def record(self, line, host_time):
        raw, checksum = line.rsplit(b'*', 1)
        if len(checksum) != 4 or int(checksum, 16) != binascii.crc_hqx(raw, 65535):
            raise ValueError('CRC')
        parts = raw.decode('ascii').split(',')
        if parts[0] != 'SW1' or any(not x.isdecimal() for x in parts[2:]):
            raise ValueError('protocol')
        values = [int(x) for x in parts[2:]]
        if any(x > 0xffffffff for x in values):
            raise ValueError('range')
        if parts[1] == 'B' and len(values) == 1:
            self.reset_flags = values[0]
            if self.rows:
                self.freeze('Observer reboot: start a new evidence session')
            return
        if parts[1] == 'P':
            if values != [20, 1, 100, 3]:
                self.freeze('Incompatible timing policy')
                raise ValueError('policy')
            self.policy = True
            return
        if parts[1] != 'S' or len(values) != len(FIELDS) or not self.policy:
            raise ValueError('status without compatible policy')
        row = dict(zip(FIELDS, values))
        self.validate(row)
        if self.frozen:
            return
        if self.rows:
            previous = self.rows[-1]
            elapsed = (row['time'] - previous['time']) & 0xffffffff
            if elapsed == 0:
                if row != previous:
                    self.freeze('Contradictory duplicate timestamp')
                return
            if elapsed >= 0x80000000 or any(row[k] < previous[k] for k in COUNTERS):
                self.freeze('Observer restart or counter rollback suspected')
                return
            if row['faults'] & previous['faults'] != previous['faults']:
                self.freeze('Sticky findings disappeared')
                return
            if previous['state'] == 2 and row['state'] != 2:
                self.freeze('Terminal fault disappeared')
                return
            if row['minimum'] > previous['minimum']:
                self.freeze('Minimum dead time increased')
                return
            if row['switches'] == previous['switches'] and row['minimum'] != previous['minimum']:
                self.freeze('Minimum changed without a new switch')
                return
            if row['overlap_ms'] - previous['overlap_ms'] > elapsed:
                self.freeze('Impossible overlap duration')
                return
        self.rows.append(row)
        self.received = host_time

    @staticmethod
    def validate(r):
        if r['state'] > 2 or r['levels'] > 15 or r['last_owner'] > 2 or r['faults'] > 511:
            raise ValueError('enum')
        if r['early'] != r['h0'] + r['h1'] or r['switches'] != sum(r[k] for k in ('h0','h1','h2','h3')):
            raise ValueError('histogram conservation')
        if (r['switches'] == 0) != (r['minimum'] == 0xffffffff):
            raise ValueError('minimum sentinel')
        if r['switches'] and ((r['early'] > 0) != (r['minimum'] < 20)):
            raise ValueError('minimum policy')
        if r['h0'] and r['minimum'] != 0:
            raise ValueError('zero bin')
        if r['switches']:
            first = next(i for i in range(4) if r['h'+str(i)])
            low, high = ((0,0),(1,19),(20,99),(100,0xfffffffe))[first]
            if not low <= r['minimum'] <= high:
                raise ValueError('minimum outside lowest occupied bucket')
        if r['overlap_ms'] and not r['overlaps']:
            raise ValueError('duration without episode')
        for field, flag in [('early',2),('overlaps',1),('unrequested',4),('gaps',8),('tx_fail',16)]:
            if r[field] and not r['faults'] & flag:
                raise ValueError('missing sticky finding')
        if r['state'] == 2:
            if not r['faults'] & 96 or r['quiet'] or r['last_owner']:
                raise ValueError('terminal state')
        elif r['faults'] & 96:
            raise ValueError('missing terminal state')
        if r['state'] == 0 and (r['last_owner'] or r['quiet'] >= 20):
            raise ValueError('baseline state')
        if r['state'] == 1 and r['levels'] >> 2 == 3:
            raise ValueError('overlap ready')
        for i,side in enumerate(('a','b')):
            peer = 'b' if side == 'a' else 'a'
            if r['starved_'+side] and not r['faults'] & (128 << i):
                raise ValueError('missing starvation finding')
            if r['bypass_'+side] > r['services_'+peer]:
                raise ValueError('bypass without peer service')
            if r['state'] != 2:
                waiting = r['levels'] & (1 << i) and not r['levels'] & (4 << i)
                if not waiting and (r['wait_'+side] or r['bypass_'+side]):
                    raise ValueError('wait without demand')
                if (r['wait_'+side] >= 100 or r['bypass_'+side] >= 3) and not r['starved_'+side]:
                    raise ValueError('missing starvation episode')

    def feed(self, data, host_time):
        # A partial frame must not consume the next valid protocol marker.
        self.buffer += data
        while b'\n' in self.buffer:
            line, self.buffer = self.buffer.split(b'\n', 1)
            marker = line.rfind(b'SW1,')
            if marker > 0:
                self.errors += 1
                line = line[marker:]
            try:
                if len(line) > 383:
                    raise ValueError('length')
                self.record(line, host_time)
            except (ValueError, UnicodeError, IndexError):
                self.errors += 1
        if len(self.buffer) > 383:
            self.errors += 1
            self.buffer = self.buffer[-3:]

    def finish(self):
        if self.buffer or not self.rows:
            raise ValueError('Replay is empty or ends with a partial frame')

    def freshness(self, now):
        if self.frozen:
            return 'FROZEN: ' + self.frozen
        if not self.connected:
            return 'OFFLINE — retained evidence'
        if self.received is None or now - self.received > 1:
            return 'STALE — no fresh status'
        return 'LIVE — sampled evidence only'


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_REPLAY:
            raise ValueError('Replay must be a regular file of at most 1 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(MAX_REPLAY + 1)
        if len(data) > MAX_REPLAY:
            raise ValueError('Replay grew beyond limit')
        return data
    finally:
        os.close(fd)


def export(model, path):
    now = time.monotonic()
    evidence = dict(author='jayis1', source=model.source, errors=model.errors,
                    frozen=model.frozen, policy=model.policy,
                    reset_flags=model.reset_flags, connected=model.connected,
                    freshness=model.freshness(now),
                    age_seconds=None if model.received is None else max(0,now-model.received),
                    rows=list(model.rows), notice='Sampled observations; not a security certification')
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as stream:
        json.dump(evidence, stream, indent=2)
        stream.write('\n')


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = None
        try:
            self.old = termios.tcgetattr(self.fd)
            new = termios.tcgetattr(self.fd)
            new[0] = 0
            new[1] = 0
            new[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            new[3] = 0
            new[4] = termios.B115200
            new[5] = termios.B115200
            new[6][termios.VMIN] = 0
            new[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, new)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except BaseException:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'', True
        data = b''
        lost = False
        for _, events in self.poller.poll(0):
            if events & select.POLLIN:
                try:
                    data = os.read(self.fd, 16384)
                except BlockingIOError:
                    pass
                except OSError:
                    lost = True
            lost |= bool(events & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data, lost

    def close(self):
        if self.fd is not None:
            try:
                if self.old is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
            except (OSError, termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd = None


class Window:
    def __init__(self, root):
        self.root = root
        self.model = Model('none')
        self.serial = None
        self.last_poll = time.monotonic()
        root.title('Starvation Window — jayis1')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.authorized = tk.BooleanVar(value=False)
        bar = ttk.Frame(root); bar.pack(fill='x')
        self.ports = ttk.Combobox(bar,textvariable=self.path,width=30)
        self.ports.pack(side='left')
        for title, command in [('Discover',self.discover),('New live',self.connect),
                               ('Disconnect',self.disconnect),('Replay',self.replay),('Export JSON',self.save)]:
            ttk.Button(bar,text=title,command=command).pack(side='left')
        ttk.Checkbutton(root,text='I have written authorization for this bench',
                        variable=self.authorized).pack(anchor='w')
        self.status = tk.StringVar()
        ttk.Label(root,textvariable=self.status).pack(anchor='w')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        self.screens = {}
        for name in ('Fairness','Snapshots','Integrity'):
            view = tk.Text(self.tabs,width=110,height=24,state='disabled')
            self.tabs.add(view,text=name)
            self.screens[name] = view
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.discover()
        self.after = root.after(50,self.poll)
        self.render()

    def discover(self):
        self.ports['values'] = sorted(set(glob.glob('/dev/ttyACM*')+glob.glob('/dev/ttyUSB*')+glob.glob('/dev/serial/by-id/*')))

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial = None
        self.model.connected = False
        self.render()

    def connect(self):
        if not self.authorized.get():
            messagebox.showerror('Authorization','Confirm authorized bench use'); return
        self.disconnect()
        try:
            serial = Serial(self.path.get())
        except (OSError,termios.error) as error:
            messagebox.showerror('Serial',str(error)); return
        self.serial = serial
        self.model = Model('live:'+self.path.get())
        self.model.connected = True
        self.last_poll = time.monotonic()
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        self.disconnect()
        candidate = Model('replay:'+path)
        try:
            candidate.feed(read_replay(path),time.monotonic())
            candidate.finish()
        except (OSError,ValueError) as error:
            messagebox.showerror('Replay',str(error)); return
        self.model = candidate
        self.render()

    def save(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if not path:
            return
        try:
            export(self.model,path)
        except (OSError,ValueError) as error:
            messagebox.showerror('Export',str(error))

    def poll(self):
        now = time.monotonic()
        if self.serial:
            # Never rejuvenate stale evidence with an unbounded OS backlog.
            if now-self.last_poll > 1 or (self.model.received is not None and now-self.model.received > 1):
                self.model.freeze('Live reception pause: reconnect to discard backlog')
                self.disconnect()
            else:
                try:
                    data,lost = self.serial.read()
                    self.model.feed(data,now)
                    if lost:
                        self.disconnect()
                except OSError:
                    self.disconnect()
        self.last_poll = now
        self.render()
        self.after = self.root.after(50,self.poll)

    def render(self):
        m = self.model
        self.status.set(m.freshness(time.monotonic()))
        latest = m.rows[-1] if m.rows else {}
        texts = {
            'Fairness': 'Fairness policy: 100 ms pending or 3 peer bypasses; secondary dead time: 20 ms\n'
                'Bits: request A, request B, enable A, enable B\n'
                'Faults: 1 overlap; 2 early; 4 unrequested; 8 gap; 16 TX; 32 watchdog; 64 exhaustion; 128 starved A; 256 starved B\n\n'
                + '\n'.join(f'{key}: {latest[key]}' for key in
                    ['wait_a','wait_b','bypass_a','bypass_b','starved_a','starved_b','services_a','services_b']
                    + [k for k in latest if not k.startswith(('wait_','bypass_','starved_','services_'))]
                    if key in latest),
            'Snapshots': '\n'.join(', '.join(f'{k}={v}' for k,v in row.items()) for row in m.rows),
            'Integrity': f'Author: jayis1\nSource: {m.source}\nParser errors: {m.errors}\n'
                f'Compatible policy: {m.policy}\nReset register: {m.reset_flags}\n'
                f'Frozen: {m.frozen}\nConnected: {m.connected}\n'
                'History bound: 256 snapshots. No payload collection or remote upload.\n'
                'A clean trace is not proof of physical isolation or absence of sub-ms overlap.'}
        for name, text in texts.items():
            view=self.screens[name]
            view.configure(state='normal'); view.delete('1.0','end'); view.insert('1.0',text)
            view.configure(state='disabled')

    def close(self):
        self.root.after_cancel(self.after)
        self.disconnect()
        self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Window(root)
    root.mainloop()
