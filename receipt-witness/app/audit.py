#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Receipt Witness: bounded receive-only Linux desktop evidence console."""
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
from tkinter import filedialog, messagebox, ttk

POLICY = [8, 50, 200, 3, 20]
FIELDS = tuple(('time ready levels count waiting faults actions appended receipts abandoned '
                'gaps missed segments max_append max_durable tx_fail').split())
COUNTERS = FIELDS[6:]
MAX_FILE = 1024 * 1024
FAULTS = ['operation while blocked', 'orphan append', 'orphan durable receipt',
          'append deadline', 'durability deadline', 'FIFO overflow',
          'acquisition gap', 'activity outside established segment']


def read_capture(path):
    # Nonblocking open plus fstat prevents a FIFO/device from hanging the UI.
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > MAX_FILE:
            raise ValueError('Replay must be a regular file of at most 1 MiB')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(MAX_FILE + 1)
        if len(data) > MAX_FILE:
            raise ValueError('Capture grew beyond 1 MiB')
        return data
    finally:
        os.close(fd)


class Model:
    def __init__(self, provenance):
        self.provenance = provenance
        self.connected = False
        self.buffer = bytearray()
        self.latest = None
        self.history = deque(maxlen=256)
        self.errors = 0
        self.compatible = False
        self.frozen = ''
        self.updated = None
        self.reset_flags = None

    def feed(self, data):
        # Streaming resynchronization tolerates truncated UART output before RW1.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'RW1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'RW1,'
            if byte == 10:
                line = bytes(self.buffer[:-1])
                self.buffer.clear()
                try:
                    self.accept(line)
                except (ValueError, UnicodeError):
                    self.errors += 1
            elif len(self.buffer) > 384:
                self.buffer[:] = self.buffer[-3:]
                self.errors += 1

    def accept(self, line):
        match = re.fullmatch(rb'(RW1,[BPS](?:,[0-9]{1,10})+)\*([0-9A-F]{4})', line)
        if not match or binascii.crc_hqx(match[1], 65535) != int(match[2], 16):
            raise ValueError('frame or CRC')
        tokens = match[1].decode('ascii').split(',')
        kind = tokens[1]
        values = [int(x) for x in tokens[2:]]
        if any(x > 0xffffffff for x in values):
            raise ValueError('integer range')
        if kind == 'B':
            if len(values) != 1:
                raise ValueError('boot field count')
            if self.latest is not None:
                self.frozen = self.frozen or 'Observer reboot after evidence'
            self.reset_flags = values[0]
            return
        if kind == 'P':
            if values != POLICY:
                self.compatible = False
                self.frozen = self.frozen or 'Incompatible acquisition policy'
            elif not self.frozen:
                self.compatible = True
            return
        if len(values) != len(FIELDS):
            raise ValueError('status field count')
        row = dict(zip(FIELDS, values))
        self.validate(row)
        if self.frozen or not self.compatible:
            return
        if self.latest is not None:
            previous = self.latest
            delta = (row['time'] - previous['time']) & 0xffffffff
            if delta == 0:
                if row != previous:
                    self.frozen = 'Contradictory duplicate timestamp'
                return
            if delta >= 0x80000000:
                self.frozen = 'Time regression or ambiguous long interval'
                return
            if any(row[k] < previous[k] for k in COUNTERS):
                self.frozen = 'Cumulative counter regression'
                return
            if row['faults'] & previous['faults'] != previous['faults']:
                self.frozen = 'Sticky finding disappeared'
                return
        self.latest = row
        self.history.append(row)
        self.updated = time.monotonic()

    @staticmethod
    def validate(r):
        if r['ready'] > 1 or r['levels'] > 15 or r['faults'] > 255:
            raise ValueError('bit range')
        if not 0 <= r['waiting'] <= r['count'] <= 8:
            raise ValueError('queue range')
        if not r['ready'] and r['count']:
            raise ValueError('queue outside segment')
        if r['ready'] and not r['segments']:
            raise ValueError('missing baseline')
        if r['max_append'] > 50 or r['max_durable'] > 200:
            raise ValueError('accepted latency exceeds policy')
        if not r['receipts'] <= r['appended'] <= r['actions']:
            raise ValueError('impossible receipts')
        # Conservation ceases to be invertible at counter saturation.
        if max(r['actions'], r['receipts'], r['abandoned']) < 0xffffffff:
            if r['actions'] != r['receipts'] + r['abandoned'] + r['count']:
                raise ValueError('operation conservation')
        if r['abandoned'] > r['actions']:
            raise ValueError('abandoned operation range')
        if max(r['actions'], r['appended'], r['receipts'], r['abandoned']) < 0xffffffff:
            lost_appends = r['appended'] - r['receipts'] - (r['count'] - r['waiting'])
            if not 0 <= lost_appends <= r['abandoned']:
                raise ValueError('append conservation')

    def integrity(self):
        age = None if self.updated is None else max(0, time.monotonic()-self.updated)
        return dict(provenance=self.provenance, connected=self.connected,
                    compatible=self.compatible, frozen=self.frozen, parser_errors=self.errors,
                    age_seconds=age, stale=age is None or age > 1,
                    reset_flags=self.reset_flags)

    def export(self, path):
        report = dict(author='jayis1', protocol='RW1', provenance=self.provenance,
                      policy=POLICY, integrity=self.integrity(), latest=self.latest,
                      history=list(self.history), limitations='Diagnostic indications; not authenticated receipts')
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(report, stream, indent=2)
            stream.write('\n')


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            if not os.isatty(self.fd):
                raise ValueError('Select an actual serial terminal')
            self.saved = termios.tcgetattr(self.fd)
            attrs = termios.tcgetattr(self.fd)
            attrs[0] = 0
            attrs[1] = 0
            attrs[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
            attrs[3] = 0
            attrs[4] = attrs[5] = termios.B115200
            attrs[6][termios.VMIN] = 0
            attrs[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            raise OSError('Serial closed')
        events = self.poller.poll(0)
        hungup = any(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL) for _, flags in events)
        try:
            data = os.read(self.fd, 4096)
        except BlockingIOError:
            data = b''
        # Some drivers preserve final readable bytes alongside HUP. Drain first;
        # next poll disconnects after the buffer empties. Others discard at HUP.
        if data:
            return data
        if hungup:
            raise OSError('Serial disconnected')
        return b''

    def close(self):
        if self.fd is not None:
            try:
                if self.saved is not None:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.saved)
            except (OSError, termios.error):
                pass
            finally:
                os.close(self.fd)
                self.fd = None


class App:
    def __init__(self, root):
        self.root = root
        root.title('Receipt Witness — jayis1')
        self.model = Model('none')
        self.serial = None
        self.path = tk.StringVar(value='/dev/ttyACM0')
        self.summary = tk.StringVar(value='No evidence')
        self.health = tk.StringVar()
        self.notice = tk.StringVar(value='Export before starting another session. Authorized bench use only.')
        toolbar = ttk.Frame(root); toolbar.pack(fill='x')
        ttk.Entry(toolbar, textvariable=self.path, width=35).pack(side='left')
        for label, command in [('New live session', self.connect), ('Disconnect', self.disconnect),
                               ('Replay', self.replay), ('Export JSON', self.export)]:
            ttk.Button(toolbar, text=label, command=command).pack(side='left')
        notebook = ttk.Notebook(root); notebook.pack(fill='both', expand=True)
        ledger = ttk.Frame(notebook); notebook.add(ledger, text='Receipt ledger')
        ttk.Label(ledger, textvariable=self.summary, justify='left').pack(anchor='w')
        timeline = ttk.Frame(notebook); notebook.add(timeline, text='History')
        self.rows = ttk.Treeview(timeline, columns=FIELDS, show='headings', height=16)
        for name in FIELDS:
            self.rows.heading(name, text=name)
            self.rows.column(name, width=76, stretch=False)
        scroll = ttk.Scrollbar(timeline, orient='horizontal', command=self.rows.xview)
        self.rows.configure(xscrollcommand=scroll.set)
        self.rows.pack(fill='both', expand=True); scroll.pack(fill='x')
        integrity = ttk.Frame(notebook); notebook.add(integrity, text='Integrity')
        ttk.Label(integrity, textvariable=self.health, justify='left').pack(anchor='w')
        ttk.Label(root, textvariable=self.notice).pack(anchor='w')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(50, self.poll)
        self.render()

    def connect(self):
        self.disconnect()
        try:
            serial = Serial(self.path.get())
            self.model = Model('live')
            self.model.connected = True
            self.serial = serial
            self.notice.set('Waiting for periodic policy and advancing status; no commands sent')
        except (OSError, ValueError, termios.error) as exc:
            self.notice.set(str(exc))
        self.render()

    def disconnect(self):
        if self.serial is not None:
            self.serial.close()
            self.serial = None
        self.model.connected = False
        self.render()

    def load_replay(self, path):
        data = read_capture(path)  # Validate before replacing existing evidence.
        self.disconnect()
        self.model = Model('replay')
        self.model.feed(data)
        self.notice.set('Replay is not live evidence; input is bounded to 1 MiB')
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load_replay(path)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Replay failed', str(exc))

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.model.export(path)
                self.notice.set('Exported: ' + path)
            except OSError as exc:
                messagebox.showerror('Export failed (existing files are never replaced)', str(exc))

    def poll(self):
        try:
            if self.serial is not None:
                self.model.feed(self.serial.read())
        except (OSError, termios.error) as exc:
            self.notice.set(str(exc))
            self.disconnect()
        self.render()
        self.timer = self.root.after(50, self.poll)

    def render(self):
        latest = self.model.latest
        text = 'No compatible status received' if latest is None else '\n'.join(f'{k}: {v}' for k,v in latest.items())
        if latest:
            names = [name for i,name in enumerate(FAULTS) if latest['faults'] & (1 << i)]
            text += '\nFindings: ' + (', '.join(names) if names else 'none observed (not a security certification)')
        self.summary.set(text)
        self.health.set('\n'.join(f'{k}: {v}' for k,v in self.model.integrity().items()))
        self.rows.delete(*self.rows.get_children())
        for row in self.model.history:
            self.rows.insert('', 'end', values=[row[k] for k in FIELDS])

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--replay', type=Path)
    parser.add_argument('--export', type=Path)
    args = parser.parse_args()
    if args.export:
        if not args.replay:
            parser.error('--export requires --replay')
        model = Model('replay')
        model.feed(read_capture(args.replay))
        model.export(args.export)
        print(json.dumps(model.integrity()))
        return
    root = tk.Tk()
    app = App(root)
    if args.replay:
        app.load_replay(args.replay)
    root.mainloop()


if __name__ == '__main__':
    main()
