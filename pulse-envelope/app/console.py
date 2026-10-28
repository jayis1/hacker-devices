#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Local receive-only Linux desktop. CRC is not authentication."""
import argparse
import binascii
from collections import deque
import copy
import json
import os
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

MAX_REPLAY = 2 * 1024 * 1024
CHANNEL_FIELDS = 'phase age starts completed shorts longs rapid aborted last peak'.split()
FIELDS = 'now levels valid halted faults gaps missed tx_fail'.split() + [
    f'c{i}_{key}' for i in range(4) for key in CHANNEL_FIELDS]
COUNTERS = ['gaps', 'missed', 'tx_fail'] + [f'c{i}_{key}' for i in range(4)
    for key in ('starts','completed','shorts','longs','rapid','aborted','peak')]
FAULTS = ('short pulse','long pulse','rapid restart','sample gap','transmit loss',
          'counter exhausted','watchdog unavailable')


def validate(v):
    if len(v) != len(FIELDS) or any(x < 0 or x > 0xffffffff for x in v):
        raise ValueError('field count/range')
    s = dict(zip(FIELDS,v))
    if s['levels'] > 15 or s['valid'] > 15 or s['halted'] > 1 or s['faults'] > 127:
        raise ValueError('enum range')
    if bool(s['halted']) != bool(s['faults'] & 96) or s['missed'] < s['gaps']:
        raise ValueError('halt/gap relationship')
    for key,bit in [('gaps',8),('tx_fail',16)]:
        if bool(s[key]) != bool(s['faults'] & bit):
            raise ValueError('diagnostic flag')
    flags=0
    for i in range(4):
        c={k:s[f'c{i}_{k}'] for k in CHANNEL_FIELDS}
        if c['phase'] > 2 or c['age'] > 201 or c['peak'] > 201:
            raise ValueError('phase/age bound')
        if (not c['phase'] and c['age']) or (c['phase']==1 and c['age']>50):
            raise ValueError('phase age')
        if c['starts'] != c['completed']+c['aborted']+(c['phase']==2):
            raise ValueError('pulse conservation')
        if c['shorts'] > c['completed'] or c['longs'] > c['starts'] or c['rapid'] > c['starts']:
            raise ValueError('finding bound')
        if c['shorts']+c['longs'] > c['starts'] or c['last'] > c['peak']:
            raise ValueError('width relationship')
        if (not c['completed'] and c['last']) or (not c['starts'] and c['peak']):
            raise ValueError('width without pulse')
        if bool(c['longs']) != (c['peak']==201):
            raise ValueError('missing long finding')
        if c['phase']==2 and c['age']>c['peak']:
            raise ValueError('current width beyond peak')
        high=bool(s['levels'] & (1<<i))
        if (c['phase'] and not s['valid'] & (1<<i)) or (c['phase']==1 and high) or (c['phase']==2 and not high):
            raise ValueError('phase versus input')
        for key,bit in [('shorts',1),('longs',2),('rapid',4)]:
            if c[key]:flags |= bit
    if flags != s['faults'] & 7:
        raise ValueError('finding flags')
    return s


class Model:
    def __init__(self, source, live):
        self.source = str(source)
        self.live_source = live
        self.connected = False
        self.policy = False
        self.boot = None
        self.status = None
        self.history = deque(maxlen=256)
        self.errors = 0
        self.buffer = bytearray()
        self.frozen = ''
        self.received = None
        self.anchor_host = None
        self.progress = 0

    def fresh(self, now):
        return bool(self.connected and self.live_source and self.policy and not self.frozen
                    and self.received is not None and 0 <= now - self.received <= 1)

    def accept(self, line, now):
        body, crc = line.rsplit(b'*', 1)
        if not re.fullmatch(rb'[0-9A-F]{4}', crc):
            raise ValueError('checksum format')
        if binascii.crc_hqx(body, 65535) != int(crc, 16):
            raise ValueError('checksum')
        parts = body.decode('ascii').split(',')
        if parts[0] != 'PE1' or len(parts) < 3:
            raise ValueError('protocol')
        if any(not re.fullmatch(r'0|[1-9][0-9]{0,9}', p) for p in parts[2:]):
            raise ValueError('decimal')
        v = [int(x) for x in parts[2:]]
        if any(x > 0xffffffff for x in v):
            raise ValueError('integer range')
        kind = parts[1]
        if kind == 'P':
            self.policy = v == [3, 10, 200, 50]
            if not self.policy:
                raise ValueError('unsupported policy')
            return
        if kind == 'B' and len(v) == 1:
            if self.status is not None:
                self.frozen = 'Boot after observation: export and start new session'
            self.boot = v[0]
            return
        if kind != 'S':
            raise ValueError('record type/layout')
        s = validate(v)
        if not self.policy or self.frozen:
            return
        if self.status is not None:
            old = self.status
            delta = (s['now'] - old['now']) & 0xffffffff
            if delta == 0:
                if s != old:
                    self.frozen = 'Contradictory duplicate'
                return
            if delta >= 0x80000000:
                self.frozen = 'Device time regressed'
                return
            if any(s[k] < old[k] for k in COUNTERS) or old['faults'] & ~s['faults']:
                self.frozen = 'Evidence counter or sticky finding regressed'
                return
            if old['halted'] and any(s[k] != old[k] for k in FIELDS if k not in ('now','tx_fail','faults')):
                self.frozen = 'Halted evidence changed'
                return
            if any(s[k] - old[k] > delta for k in tuple(k for k in COUNTERS if k not in ('missed','tx_fail') and not k.endswith('_peak'))):
                self.frozen = 'Impossible episode increment'
                return
            if any(s[k] > old[k] + delta for k in tuple(f'c{i}_peak' for i in range(4))):
                self.frozen = 'Impossible duration increment'
                return
            if self.live_source:
                if now - self.received > 1:
                    self.frozen = 'Host pause: reconnect to discard backlog'
                    return
                self.progress += delta
                host_ms = (now - self.anchor_host) * 1000
                if abs(self.progress - host_ms) > 500 + host_ms * .05:
                    self.frozen = 'Host/device clock divergence or serial backlog'
                    return
        else:
            self.anchor_host = now
        self.status = s
        self.received = now
        self.history.append(dict(s))

    def feed(self, data, now):
        # Byte-wise bounded framing also recovers a truncated line at a full marker.
        for ch in data:
            self.buffer.append(ch)
            if self.buffer.endswith(b'PE1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'PE1,'
            if ch == 10:
                line = bytes(self.buffer[:-1])
                self.buffer.clear()
                try:
                    self.accept(line, now)
                except (ValueError, UnicodeError):
                    self.errors += 1
            elif len(self.buffer) >= 768:
                self.errors += 1
                self.buffer[:] = self.buffer[-3:]

    def eof(self):
        if self.buffer:
            self.errors += 1
            self.buffer.clear()
        if self.status is None:
            self.errors += 1

    def report(self, now):
        return dict(author='jayis1', protocol='PE1', source=self.source,
                    provenance='live serial' if self.live_source else 'replay (not live)',
                    connected=self.connected, fresh=self.fresh(now), policy=self.policy,
                    age_seconds=None if self.received is None else now-self.received,
                    freeze_reason=self.frozen, parser_errors=self.errors, boot_flags=self.boot,
                    status=self.status, history=list(self.history))

    def export(self, path, now):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        with os.fdopen(fd, 'w') as f:
            json.dump(self.report(now), f, indent=2)
            f.write('\n')


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_REPLAY:
            raise ValueError('Replay must be a regular file at most 2 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as f:
            data = f.read(MAX_REPLAY + 1)
        if len(data) > MAX_REPLAY:
            raise ValueError('Replay grew beyond limit')
        return data
    finally:
        os.close(fd)


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = None
        try:
            self.old = termios.tcgetattr(self.fd)
            attrs = copy.deepcopy(self.old)
            attrs[0] = attrs[1] = attrs[3] = 0
            attrs[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
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
        dead = False
        for _, flags in self.poller.poll(0):
            if flags & select.POLLIN:
                try:
                    data = os.read(self.fd, 8192)
                except BlockingIOError:
                    pass
            dead = bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))
        return data, dead

    def close(self):
        if self.fd is None:
            return
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
        self.serial = None
        self.model = Model('none', False)
        root.title('Pulse Envelope — jayis1 — authorized lab use only')
        self.port = tk.StringVar(value='/dev/ttyACM0')
        toolbar = ttk.Frame(root); toolbar.pack(fill='x')
        ttk.Entry(toolbar, textvariable=self.port, width=28).pack(side='left')
        for label, command in [('New live session', self.connect), ('Disconnect', self.disconnect),
                               ('Replay', self.replay_dialog), ('Export', self.export_dialog)]:
            ttk.Button(toolbar, text=label, command=command).pack(side='left')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True)
        self.views = {}
        for name in ('Envelope', 'History', 'Integrity'):
            frame = ttk.Frame(self.tabs); self.tabs.add(frame, text=name)
            view = tk.Text(frame, width=100, height=24, state='disabled')
            view.pack(fill='both', expand=True); self.views[name] = view
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.draw()
        self.timer = root.after(40, self.poll)

    def connect(self):
        self.disconnect()
        self.model = Model(self.port.get(), True)
        try:
            self.serial = Serial(self.port.get())
            self.model.connected = True
        except (OSError, termios.error) as exc:
            self.model.frozen = str(exc)
        self.draw()

    def disconnect(self):
        if self.serial:
            self.serial.close(); self.serial = None
        self.model.connected = False
        self.draw()

    def replay(self, path):
        self.disconnect()
        self.model = Model(path, False)
        try:
            self.model.feed(read_replay(path), time.monotonic())
            self.model.eof()
        except (OSError, ValueError) as exc:
            self.model.frozen = str(exc)
        self.draw()

    def replay_dialog(self):
        path = filedialog.askopenfilename(title='Replace session with replay (export first)')
        if path:
            self.replay(path)

    def export_dialog(self):
        path = filedialog.asksaveasfilename(title='New private JSON report', defaultextension='.json')
        if path:
            try:
                self.model.export(path, time.monotonic())
            except OSError as exc:
                messagebox.showerror('Export refused', str(exc))

    def draw(self):
        m = self.model
        s = m.status
        status = 'LIVE transport' if m.fresh(time.monotonic()) else 'NOT LIVE'
        text = status + '\nAuthor: jayis1\n'
        if s:
            text += 'Measurement: ' + ('HALTED' if s['halted'] else 'QUALIFIED' if s['valid']==15 else 'UNKNOWN') + '\n'
            for i, label in enumerate(('CH0', 'CH1', 'CH2', 'CH3')):
                text += f'{label}: {(s["levels"]>>i)&1} (qualified: {bool(s["valid"]&(1<<i))})\n'
                c = {k:s[f'c{i}_{k}'] for k in CHANNEL_FIELDS}
                text += '  ' + '  '.join(f'{k}={c[k]}' for k in ('phase','age','starts','completed','aborted')) + '\n'
                text += '  ' + '  '.join(f'{k}={c[k]}' for k in ('shorts','longs','rapid','last','peak')) + '\n'
            text += 'Widths of 201 mean >=201 ms, not exact duration.\n'
            text += f'Gaps={s["gaps"]} missed={s["missed"]} TX failures={s["tx_fail"]}\n'
            text += 'Sticky findings: ' + ', '.join(name for i,name in enumerate(FAULTS) if s['faults'] & (1<<i))
        else:
            text += 'Waiting for compatible policy and status; no measurements.\n'
        report = m.report(time.monotonic()); report.pop('history'); report.pop('status')
        texts = {'Envelope': text,
                 'History': '\n'.join(str(row) for row in m.history),
                 'Integrity': json.dumps(report, indent=2)}
        for name, content in texts.items():
            view=self.views[name]; view.configure(state='normal')
            view.delete('1.0','end'); view.insert('1.0',content); view.configure(state='disabled')

    def poll(self):
        if self.serial:
            try:
                data, dead = self.serial.read()
                if data:
                    self.model.feed(data, time.monotonic())
                if dead:
                    self.disconnect()
            except (OSError, termios.error) as exc:
                self.model.frozen = str(exc)
                self.disconnect()
        self.draw()
        self.timer = self.root.after(40, self.poll)

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay')
    parser.add_argument('--export')
    args=parser.parse_args()
    if args.replay:
        m=Model(args.replay,False)
        m.feed(read_replay(args.replay),time.monotonic()); m.eof()
        if args.export:
            m.export(args.export,time.monotonic())
        print(json.dumps(m.report(time.monotonic()),indent=2))
        return int(bool(m.errors or m.frozen or m.status is None))
    root=tk.Tk(); Window(root); root.mainloop()
    return 0
if __name__=='__main__':
    raise SystemExit(main())
