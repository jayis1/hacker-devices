#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Local Linux evidence console; no network and no target commands."""
import argparse
import binascii
from collections import deque
import json
import os
from pathlib import Path
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

FIELDS = ('time phase qualified levels valid have_highest highest current capture_age '
          'faults accepted downgrades invalid execute_ms gaps missed segments dropped tx_fail').split()
COUNTERS = FIELDS[10:]
POLICY = [10, 10, 1000, 3, 20]
PHASES = ['Unknown', 'Ready', 'Capture']
FAULTS = ['Setup violation', 'Generation changed during commit', 'Commit too short',
          'Commit timeout', 'Generation rollback', 'Unannounced execution',
          'Acquisition gap', 'Startup activity']

class Model:
    def __init__(self, source):
        self.source = source
        self.connected = False
        self.buffer = b''
        self.latest = None
        self.events = deque(maxlen=256)
        self.errors = 0
        self.discarded = 0
        self.compatible = False
        self.blocked = False
        self.warning = 'Waiting for policy and advancing status'
        self.updated = None
        self.last_event = None
        self.event_gaps = 0

    def freeze(self, reason):
        self.blocked = True
        self.warning = reason + '; export then start a new session'

    def feed(self, data):
        # Bound memory even for callers supplying huge chunks. Resynchronize on
        # the complete marker, including a truncated previous frame without LF.
        for byte in data:
            self.buffer += bytes((byte,))
            if self.buffer.endswith(b'EW1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer = b'EW1,'
            if byte == 10:
                line, self.buffer = self.buffer, b''
                try:
                    self.accept(line)
                except (ValueError, UnicodeError):
                    self.errors += 1
            elif len(self.buffer) >= 384:
                self.buffer = b''
                self.errors += 1

    def accept(self, raw):
        if not re.fullmatch(rb'EW1,[BPSE],[0-9,]+\*[0-9A-F]{4}\n', raw):
            raise ValueError('frame syntax')
        body, crc = raw[:-1].split(b'*')
        if binascii.crc_hqx(body, 65535) != int(crc, 16):
            raise ValueError('CRC')
        parts = body.decode('ascii').split(',')
        kind = parts[1]
        values = [int(x) for x in parts[2:]]
        if any(x > 0xffffffff for x in values):
            raise ValueError('range')
        required = {'B': 1, 'P': 5, 'S': len(FIELDS), 'E': 4}
        if kind not in required or len(values) != required[kind]:
            raise ValueError('field count')
        if kind == 'P':
            if values != POLICY:
                self.compatible = False
                self.freeze('Incompatible policy')
            elif not self.blocked:
                self.compatible = True
            return
        if kind == 'B':
            if self.latest is not None or self.last_event is not None:
                self.freeze('Device boot observed')
            return
        if kind == 'E' and not 1 <= values[2] <= 7:
            raise ValueError('event kind')
        if kind == 'E':
            event_kind, value = values[2:]
            if ((event_kind in (2, 3, 4, 5) and value > 3)
                    or (event_kind == 6 and not 1 <= value <= 255)
                    or (event_kind in (1, 7) and value == 0)):
                raise ValueError('event value')
        v = {}
        if kind == 'S':
            v = dict(zip(FIELDS, values))
            if (v['phase'] > 2 or v['qualified'] > 1 or v['levels'] > 15
                    or v['valid'] > 1 or v['have_highest'] > 1
                    or v['highest'] > 3 or v['current'] > 3 or v['faults'] > 255
                    or v['capture_age'] > 1001):
                raise ValueError('status range')
            if v['valid'] and (not v['have_highest'] or v['current'] != v['highest']
                               or v['phase'] != 1 or not v['accepted']
                               or v['levels'] & 4 or (v['levels'] & 3) != v['current']):
                raise ValueError('inconsistent validity')
            if v['phase'] and not v['qualified']:
                raise ValueError('phase without qualified inputs')
        if self.blocked or not self.compatible:
            self.discarded += 1
            return
        if kind == 'E':
            seq = values[0]
            if self.last_event is not None:
                delta = (seq - self.last_event) & 0xffffffff
                if delta == 0:
                    if values != self.events[-1]:
                        self.freeze('Contradictory same-sequence event')
                    return
                if delta >= 0x80000000:
                    self.freeze('Event sequence regression')
                    return
                self.event_gaps += delta - 1
            self.last_event = seq
            self.events.append(values)
            return
        if self.latest:
            delta = (v['time'] - self.latest['time']) & 0xffffffff
            if not delta:
                if v != self.latest:
                    self.freeze('Contradictory same-time status')
                return  # duplicates never refresh freshness
            if delta >= 0x80000000:
                self.freeze('Device time regression')
                return
            if any(v[k] < self.latest[k] for k in COUNTERS):
                self.freeze('Cumulative counter regression')
                return
            if self.latest['faults'] & ~v['faults']:
                self.freeze('Sticky faults disappeared')
                return
            if self.latest['have_highest'] and (not v['have_highest'] or v['highest'] < self.latest['highest']):
                self.freeze('High water mark regressed')
                return
            for key in ('execute_ms',):
                if v[key] - self.latest[key] > delta:
                    raise ValueError('impossible exposure increment')
        self.latest = v
        self.updated = time.monotonic()
        self.warning = 'No physical authentication; diagnostic indications only'

    def snapshot(self):
        age = None if self.updated is None else time.monotonic() - self.updated
        return dict(author='jayis1', protocol='EW1', source=self.source,
                    connected=self.connected, compatible=self.compatible,
                    blocked=self.blocked, warning=self.warning, age_seconds=age,
                    stale=age is None or age > 1, errors=self.errors,
                    discarded=self.discarded, event_gaps=self.event_gaps,
                    latest=self.latest, events=list(self.events))

    def export(self, path):
        # Exclusive creation protects existing reports; no symlink following.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(self.snapshot(), stream, indent=2)
            stream.write('\n')

class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.old = None
        try:
            if not stat.S_ISCHR(os.fstat(self.fd).st_mode) or not os.isatty(self.fd):
                raise OSError('Select a serial terminal device')
            self.old = termios.tcgetattr(self.fd)
            attr = termios.tcgetattr(self.fd)
            attr[0] = attr[1] = attr[3] = 0
            attr[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attr[4] = attr[5] = termios.B115200
            attr[6][termios.VMIN] = 0
            attr[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attr)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial closed')
        for _, flags in self.poller.poll(0):
            if flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
                raise OSError('Serial disconnected')
        try:
            return os.read(self.fd, 4096)
        except BlockingIOError:
            return b''

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

class Console:
    def __init__(self, root):
        self.root = root
        self.model = Model('none')
        self.serial = None
        self.closed = False
        root.title('Epoch Witness — jayis1 — unvalidated prototype')
        root.geometry('960x640')
        controls = ttk.Frame(root); controls.pack(fill='x')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        ttk.Entry(controls,textvariable=self.path,width=30).pack(side='left')
        for label, action in [('New live session',self.connect),('Disconnect',self.disconnect),
                              ('Replay',self.replay),('Export JSON',self.export)]:
            ttk.Button(controls,text=label,command=action).pack(side='left')
        self.banner = tk.StringVar()
        ttk.Label(root,textvariable=self.banner,wraplength=940).pack(fill='x')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both',expand=True)
        self.views = {}
        for title in ['Epochs', 'Events', 'Integrity']:
            text = tk.Text(self.tabs,wrap='word',state='disabled')
            self.tabs.add(text,text=title); self.views[title]=text
        root.protocol('WM_DELETE_WINDOW',self.close)
        self.poll()

    def disconnect(self):
        if self.serial:
            self.serial.close()
        self.serial = None
        self.model.connected = False

    def connect(self):
        self.disconnect()
        self.model = Model('live')
        try:
            self.serial = Serial(self.path.get())
            self.model.connected = True
        except (OSError, termios.error) as exc:
            self.model.warning = str(exc)
        self.render()

    def load_replay(self, path):
        self.disconnect()
        self.model = Model('replay')
        try:
            data = read_replay(path)
            self.model.feed(data)
        except (OSError, ValueError) as exc:
            self.model.warning = str(exc)
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if path:
            self.load_replay(path)

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.model.export(path)
            except OSError as exc:
                messagebox.showerror('Export refused',str(exc))

    def render(self):
        snap = self.model.snapshot()
        state = 'CONNECTED' if snap['connected'] else 'DISCONNECTED'
        self.banner.set(f"{snap['source']} / {state} / "
                        f"{'STALE' if snap['stale'] else 'recent receipt'} / {snap['warning']}")
        v = self.model.latest
        summary = 'No accepted status. Waiting for compatible EW1 policy.'
        if v:
            summary = f"Phase: {PHASES[v['phase']]}\n"
            summary += f"Highest observed: {v['highest']} (known: {v['have_highest']})\n"
            summary += f"Current announcement valid: {v['valid']}\n\n"
            summary += '\n'.join(f'{key}: {v[key]}' for key in FIELDS)
            summary += '\n\nSticky findings:\n' + '\n'.join(
                name for i,name in enumerate(FAULTS) if v['faults'] & (1 << i))
        contents = {'Epochs':summary,
                    'Events':'Sequence | device ms | kind | value\n'+'\n'.join(
                        ' | '.join(map(str,e)) for e in self.model.events),
                    'Integrity':json.dumps({k:v for k,v in snap.items() if k not in ('events','latest')},indent=2)}
        for name,text in contents.items():
            view=self.views[name]
            view.configure(state='normal'); view.delete('1.0','end')
            view.insert('1.0',text); view.configure(state='disabled')

    def poll(self):
        if self.closed:
            return
        if self.serial:
            try:
                self.model.feed(self.serial.read())
            except (OSError, termios.error) as exc:
                self.disconnect()
                self.model.warning = str(exc)
        self.render()
        self.root.after(50,self.poll)

    def close(self):
        self.closed=True
        self.disconnect()
        self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay',type=Path)
    parser.add_argument('--export',type=Path,help='Headless replay export; refuses overwrite')
    args=parser.parse_args()
    if args.export:
        if not args.replay:
            parser.error('--export requires --replay')
        data=read_replay(args.replay)
        model=Model('replay'); model.feed(data); model.export(args.export)
        return
    root=tk.Tk(); console=Console(root)
    if args.replay:
        console.load_replay(args.replay)
    root.mainloop()

def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    with os.fdopen(fd, 'rb') as stream:
        if not stat.S_ISREG(os.fstat(stream.fileno()).st_mode):
            raise ValueError('Replay requires a regular file')
        data = stream.read(2 * 1024 * 1024 + 1)
    if len(data) > 2 * 1024 * 1024:
        raise ValueError('Replay exceeds 2 MiB limit')
    return data

if __name__=='__main__':
    main()
