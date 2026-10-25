#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Latency Pair: bounded receive-only Linux/Tk timing evidence console."""
import binascii
from collections import deque
import json
import os
from pathlib import Path
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = 'time state levels pairs sum_a sum_b a_slower b_slower equal large max_delta rejected gaps tx_fail faults'.split()
LIFETIME = 'pairs sum_a sum_b a_slower b_slower equal large max_delta rejected gaps tx_fail'.split()
FAULTS = ['unstable class','invalid trial','inhibited trial','timeout','repeated class','acquisition gap','serial loss','watchdog unavailable']


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > 1048576:
            raise ValueError('Replay must be a regular file at most 1 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as f:
            data = f.read(1048577)
        if len(data) > 1048576:
            raise ValueError('Replay grew beyond limit')
        return data
    finally:
        os.close(fd)


class Model:
    def __init__(self, provenance):
        self.provenance = provenance
        self.buffer = bytearray()
        self.rows = deque(maxlen=256)
        self.errors = 0
        self.policy = False
        self.frozen = ''
        self.updated = None
        self.reset_flags = None

    def feed(self, data):
        # Incremental marker search handles truncated frames without a newline.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'LP1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'LP1,'
            if len(self.buffer) > 384:
                self.errors += 1
                self.buffer.clear()
            if byte == 10 and self.buffer:
                line = bytes(self.buffer).rstrip(b'\r\n')
                self.buffer.clear()
                try:
                    self.accept(line)
                except (ValueError, UnicodeError):
                    self.errors += 1

    def finish(self):
        if self.buffer:
            self.errors += 1
            self.buffer.clear()
            self.frozen = 'Truncated recording tail'

    def accept(self, line):
        body, checksum = line.rsplit(b'*', 1)
        if len(checksum) != 4 or any(c not in b'0123456789ABCDEF' for c in checksum):
            raise ValueError('CRC format')
        if binascii.crc_hqx(body, 65535) != int(checksum, 16):
            raise ValueError('CRC mismatch')
        fields = body.decode('ascii').split(',')
        if fields[0] != 'LP1' or len(fields) < 3:
            raise ValueError('Protocol')
        if any(not v.isascii() or not v.isdecimal() or len(v)>10 for v in fields[2:]):
            raise ValueError('Integer format')
        values = list(map(int, fields[2:]))
        if any(v > 0xffffffff for v in values):
            raise ValueError('Integer range')
        kind = fields[1]
        if kind == 'B' and len(values) == 1:
            if self.rows:
                self.frozen = 'Reset record after measurements; start a new session'
            self.reset_flags = values[0]
        elif kind == 'P':
            if values != [1000,10000,32,5]:
                self.frozen = 'Incompatible policy'
                raise ValueError('Policy')
            self.policy = True
        elif kind == 'S' and len(values) == len(FIELDS):
            self.status(dict(zip(FIELDS,values)))
        else:
            raise ValueError('Record type or field count')

    def status(self, r):
        if not self.policy or self.frozen:
            return
        n = r['pairs']
        if not (n <= 32 and r['state'] <= 3 and r['levels'] <= 15 and r['faults'] <= 255):
            raise ValueError('Range')
        if r['a_slower'] + r['b_slower'] + r['equal'] != n or r['large'] > n:
            raise ValueError('Pair conservation')
        if any(not n <= r[k] <= n*1000 for k in ('sum_a','sum_b')):
            raise ValueError('Duration bounds')
        if r['max_delta'] >= 1000 or (not n and r['max_delta']):
            raise ValueError('Difference bounds')
        if bool(r['large']) != (r['max_delta'] >= 5):
            raise ValueError('Threshold mismatch')
        unequal = r['a_slower'] + r['b_slower']
        if r['large'] > unequal or (unequal == 0) != (r['max_delta'] == 0):
            raise ValueError('Tie/difference mismatch')
        difference = r['sum_b'] - r['sum_a']
        lower = r['b_slower'] - r['a_slower']*r['max_delta']
        upper = r['b_slower']*r['max_delta'] - r['a_slower']
        if not lower <= difference <= upper:
            raise ValueError('Direction/sum mismatch')
        if n == 1 and abs(difference) != r['max_delta']:
            raise ValueError('Single-pair difference mismatch')
        if r['state'] == 2 and r['levels'] & 13 != 5:
            raise ValueError('Active input mismatch')
        if r['state'] == 1 and r['levels'] & 13 != 4:
            raise ValueError('Ready input mismatch')
        if (r['state'] == 3) != (n == 32):
            raise ValueError('Cohort state')
        if (r['gaps'] and not r['faults'] & 32) or (r['tx_fail'] and not r['faults'] & 64):
            raise ValueError('Unrecorded failure')
        if r['rejected'] and not r['faults'] & 63:
            raise ValueError('Unrecorded rejection')
        if r['faults'] & 128 and (n or r['state']):
            raise ValueError('Measurement with failed watchdog')
        if self.rows:
            old = self.rows[-1]
            delta = (r['time'] - old['time']) & 0xffffffff
            if delta == 0:
                if r != old:
                    self.frozen = 'Contradictory duplicate timestamp'
                return
            if delta >= 0x80000000 or any(r[k] < old[k] for k in LIFETIME) or old['faults'] & ~r['faults']:
                self.frozen = 'Possible observer reset or counter rollback'
                return
            added = n - old['pairs']
            if any(not added <= r[k]-old[k] <= added*1000 for k in ('sum_a','sum_b')):
                raise ValueError('Impossible pair sums')
            if not added and r['max_delta'] != old['max_delta']:
                raise ValueError('Difference without pair')
        self.rows.append(r)
        self.updated = time.monotonic()

    def evidence(self, connected):
        age = None if self.updated is None else max(0,time.monotonic()-self.updated)
        return dict(author='jayis1', protocol='LP1', provenance=self.provenance,
                    connected=connected, policy=self.policy, errors=self.errors,
                    frozen=self.frozen, reset_flags=self.reset_flags, age_seconds=age,
                    stale=not connected or age is None or age>1,
                    rows=list(self.rows))

    def export(self, path, connected):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd,'w') as f:
            json.dump(self.evidence(connected),f,indent=2)
            f.write('\n')


class Serial:
    def __init__(self, path):
        self.fd = os.open(path,os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            attrs = termios.tcgetattr(self.fd)
            attrs[0] = attrs[1] = attrs[3] = 0
            attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd,termios.TCSANOW,attrs)
            termios.tcflush(self.fd,termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd,select.POLLIN | select.POLLHUP | select.POLLERR)
        except BaseException:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'', True
        events = self.poller.poll(0)
        data = b''
        gone = False
        for _,flags in events:
            if flags & select.POLLIN:
                try:
                    data += os.read(self.fd,65536)
                except BlockingIOError:
                    pass
            gone |= bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data,gone

    def close(self):
        if self.fd is None:
            return
        try:
            if self.saved is not None:
                termios.tcsetattr(self.fd,termios.TCSANOW,self.saved)
        except (OSError,termios.error):
            pass
        finally:
            os.close(self.fd)
            self.fd = None


class Console:
    def __init__(self, root):
        self.root = root
        self.model = Model('none')
        self.serial = None
        self.notice = 'No capture'
        root.title('Latency Pair / jayis1')
        bar = ttk.Frame(root)
        bar.pack(fill='x')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(bar,textvariable=self.path,width=28).pack(side='left')
        for label,command in [('New live',self.connect),('Disconnect',self.disconnect),('Replay',self.replay),('Export',self.export)]:
            ttk.Button(bar,text=label,command=command).pack(side='left')
        self.tabs = ttk.Notebook(root)
        self.tabs.pack(fill='both',expand=True)
        self.views = {}
        for name in ('Pairs','History','Integrity'):
            view = tk.Text(self.tabs,width=110,height=24,state='disabled')
            self.tabs.add(view,text=name)
            self.views[name] = view
        self.root.protocol('WM_DELETE_WINDOW',self.close)
        self.timer = root.after(25,self.poll)
        self.render()

    def connect(self):
        try:
            self.disconnect()
            port = Serial(self.path.get())
            self.serial = port
            self.model = Model('live: '+self.path.get())
            self.notice = 'Connected; waiting for periodic policy'
        except (OSError,termios.error) as exc:
            self.notice = str(exc)
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
            self.serial = None
        self.notice = 'Disconnected; evidence retained'
        self.render()

    def load(self,path):
        data = read_replay(path)
        replacement = Model('replay: '+str(path))
        replacement.feed(data)
        replacement.finish()
        self.disconnect()
        self.model = replacement
        self.notice = 'Replay, not live measurements'
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load(path)
            except (ValueError,OSError) as exc:
                messagebox.showerror('Replay rejected',str(exc))

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.model.export(path,self.serial is not None)
            except OSError as exc:
                messagebox.showerror('Export refused',str(exc))

    def poll(self):
        if self.serial:
            try:
                data,gone = self.serial.read()
                self.model.feed(data)
                if gone:
                    self.disconnect()
                    self.notice = 'Serial hangup; evidence retained'
            except (OSError,termios.error) as exc:
                self.disconnect()
                self.notice = str(exc)
        self.render()
        self.timer = self.root.after(25,self.poll)

    def render(self):
        evidence = self.model.evidence(self.serial is not None)
        latest = self.model.rows[-1] if self.model.rows else None
        summary = 'No compatible measurements.\n'
        if latest:
            findings = [name for i,name in enumerate(FAULTS) if latest['faults'] & (1<<i)]
            n = latest['pairs']
            mean = (latest['sum_b']-latest['sum_a'])/n if n else None
            summary = (f"Completed pairs: {n} / 32\n"
                       f"State: {['waiting baseline','ready','measuring','cohort complete'][latest['state']]}\n"
                       f"Mean B minus A: {mean if mean is not None else 'unavailable'} sampled ms\n"
                       f"A slower / B slower / ties: {latest['a_slower']} / {latest['b_slower']} / {latest['equal']}\n"
                       f"Pairs differing by at least 5 ms: {latest['large']}\n"
                       f"Maximum absolute pair difference: {latest['max_delta']} ms\n"
                       f"Rejected candidate pairs: {latest['rejected']}\n"
                       f"Findings: {', '.join(findings) or 'none observed'}\n")
        summary += f"\n{self.notice}\nStale: {evidence['stale']}\nFrozen: {self.model.frozen or 'no'}\n"
        summary += '\nDescriptive timing evidence, not a constant-time certificate or significance test.'
        history = '\t'.join(FIELDS)+'\n'+'\n'.join('\t'.join(str(r[k]) for k in FIELDS) for r in self.model.rows)
        evidence.pop('rows')
        for name,text in [('Pairs',summary),('History',history),('Integrity',json.dumps(evidence,indent=2))]:
            view = self.views[name]
            view.configure(state='normal')
            view.delete('1.0','end')
            view.insert('1.0',text)
            view.configure(state='disabled')

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


if __name__ == '__main__':
    root = tk.Tk()
    Console(root)
    root.mainloop()
