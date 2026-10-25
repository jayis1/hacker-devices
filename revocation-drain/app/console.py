#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Offline Linux observation console. No target commands or network services."""
import argparse
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

LIMIT = 1024 * 1024
FIELDS = tuple('time state levels started completed abandoned age quiet admissions blocked early gaps tx_fail faults'.split())
COUNTERS = 'started completed abandoned admissions blocked early gaps tx_fail'.split()
STATES = ['Waiting baseline', 'Ready', 'Draining', 'Acknowledged', 'Terminal fault']
FAULTS = ['Admission during revocation', 'Early acknowledgment', 'Drain timeout',
          'Revocation withdrawn', 'Activity after acknowledgment', 'Acquisition gap',
          'Transmit loss', 'Watchdog unavailable', 'Counter exhausted', 'Orphan acknowledgment']


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


def valid(v):
    s = v['state']
    if s > 4 or v['levels'] > 15 or v['faults'] > 1023:
        return False
    if v['started'] != v['completed'] + v['abandoned'] + (s == 2):
        return False
    if v['age'] > 500 or v['quiet'] > 20:
        return False
    if s != 2 and (v['age'] or v['quiet']):
        return False
    if s == 2 and (not v['levels'] & 1 or (v['levels'] & 4 and v['quiet'])):
        return False
    if s == 3 and (not v['completed'] or not v['levels'] & 1):
        return False
    for key, bit in [('blocked', 1), ('early', 2), ('gaps', 32), ('tx_fail', 64)]:
        if v[key] and not v['faults'] & bit:
            return False
    return (s == 4) == bool(v['faults'] & (128 | 256))


class Model:
    def __init__(self, source):
        self.source = source
        self.buffer = bytearray()
        self.policy = False
        self.reset = None
        self.latest = None
        self.rows = deque(maxlen=256)
        self.errors = 0
        self.error = ''
        self.arrival = None
        self.connected = False

    def freeze(self, reason):
        self.error = reason

    def feed(self, data):
        # Bound storage even when a hostile peer never sends LF.
        for byte in data:
            self.buffer.append(byte)
            if self.buffer.endswith(b'RD1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer = bytearray(b'RD1,')
            if byte == 10:
                packet = bytes(self.buffer[:-1])
                self.buffer.clear()
                self.packet(packet)
            elif len(self.buffer) > 383:
                self.errors += 1
                self.buffer.clear()

    def packet(self, packet):
        if self.error:
            return
        match = re.fullmatch(rb'(RD1,[BPS](?:,[0-9]{1,10})+)\*([0-9A-F]{4})', packet)
        if not match or crc(match[1]) != int(match[2], 16):
            self.errors += 1
            return
        fields = match[1].decode('ascii').split(',')
        values = list(map(int, fields[2:]))
        if any(x > 0xffffffff for x in values):
            self.freeze('Integer out of range')
            return
        kind = fields[1]
        if kind == 'B':
            if len(values) != 1:
                self.freeze('Malformed boot record')
            elif self.latest is not None:
                self.freeze('Observer reboot: start a new session explicitly')
            else:
                self.reset = values[0]
            return
        if kind == 'P':
            if values != [20, 500]:
                self.freeze('Incompatible policy')
            else:
                self.policy = True
            return
        if not self.policy:
            self.errors += 1
            return
        if len(values) != len(FIELDS):
            self.freeze('Malformed status length')
            return
        v = dict(zip(FIELDS, values))
        if not valid(v):
            self.freeze('Impossible status invariant')
            return
        old = self.latest
        if old:
            delta = (v['time'] - old['time']) & 0xffffffff
            if delta == 0:
                if v != old:
                    self.freeze('Contradictory duplicate timestamp')
                return
            if delta >= 0x80000000 or any(v[k] < old[k] for k in COUNTERS):
                self.freeze('Clock/counter rollback; possible observer reset')
                return
            if old['faults'] & ~v['faults']:
                self.freeze('Sticky fault disappeared')
                return
            if old['state'] == 4 and v['state'] != 4:
                self.freeze('Terminal fault cleared without new session')
                return
        self.latest = v
        self.rows.append(v)
        self.arrival = time.monotonic()

    def finish(self):
        if self.buffer:
            self.freeze('Truncated replay tail')
        elif not self.latest:
            self.freeze('No accepted status in replay')

    def age(self):
        return None if self.arrival is None else max(0, time.monotonic() - self.arrival)

    def report(self):
        return dict(author='jayis1', protocol='RD1', source=self.source,
                    connected=self.connected, policy=self.policy, reset_flags=self.reset,
                    parser_errors=self.errors, frozen_reason=self.error, age_seconds=self.age(),
                    latest=self.latest, history=list(self.rows),
                    warning='Sampled indications, not authenticated evidence or enforcement')


def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > LIMIT:
            raise ValueError('Replay must be a regular file of at most one MiB')
        data = bytearray()
        while len(data) <= LIMIT:
            part = os.read(fd, min(65536, LIMIT + 1 - len(data)))
            if not part:
                break
            data.extend(part)
        if len(data) > LIMIT:
            raise ValueError('Replay grew beyond size limit')
        return bytes(data)
    finally:
        os.close(fd)


def export(model, path):
    # Exclusive creation prevents silently destroying an earlier evidence file.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as stream:
        json.dump(model.report(), stream, indent=2)
        stream.write('\n')


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.saved = None
        try:
            self.saved = termios.tcgetattr(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[0] = 0
            settings[1] = 0
            settings[2] = termios.CS8 | termios.CLOCAL | termios.CREAD
            settings[3] = 0
            settings[4] = settings[5] = termios.B115200
            settings[6][termios.VMIN] = 0
            settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except BaseException:
            self.close()
            raise

    def read(self):
        if self.fd is None:
            return b'', True
        flags = 0
        for _, event in self.poller.poll(0):
            flags |= event
        data = b''
        if flags & select.POLLIN:
            try:
                data = os.read(self.fd, 4096)
            except BlockingIOError:
                pass
        # Return final readable bytes before the application handles hangup.
        return data, bool(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL))

    def close(self):
        if self.fd is None:
            return
        try:
            if self.saved is not None:
                termios.tcsetattr(self.fd, termios.TCSANOW, self.saved)
        except (OSError, termios.error):
            pass
        finally:
            os.close(self.fd)
            self.fd = None


class Window:
    def __init__(self, root):
        self.root = root
        self.serial = None
        self.model = Model('No session')
        root.title('Revocation Drain — jayis1')
        root.geometry('1050x650')
        self.authorized = tk.BooleanVar(value=False)
        ttk.Checkbutton(root, text='I have written authorization for this bench and wiring',
                        variable=self.authorized).pack(anchor='w')
        controls = ttk.Frame(root); controls.pack(fill='x')
        self.port = ttk.Entry(controls, width=35); self.port.pack(side='left')
        self.port.insert(0, '/dev/ttyACM0')
        for label, command in [('New live', self.connect), ('Disconnect', self.disconnect),
                               ('Replay', self.replay), ('Export JSON', self.save)]:
            ttk.Button(controls, text=label, command=command).pack(side='left')
        self.status = tk.StringVar(value='No session')
        ttk.Label(root, textvariable=self.status).pack(anchor='w')
        tabs = ttk.Notebook(root); tabs.pack(expand=True, fill='both')
        self.summary = tk.Text(tabs, wrap='word', height=20)
        self.history = ttk.Treeview(tabs, columns=FIELDS, show='headings')
        for name in FIELDS:
            self.history.heading(name, text=name)
            self.history.column(name, width=70, stretch=True)
        self.integrity = tk.Text(tabs, wrap='word', height=20)
        tabs.add(self.summary, text='Drain policy')
        tabs.add(self.history, text='Snapshots')
        tabs.add(self.integrity, text='Integrity')
        root.protocol('WM_DELETE_WINDOW', self.close)
        self.timer = root.after(25, self.poll)
        self.render()

    def connect(self):
        if not self.authorized.get():
            messagebox.showerror('Authorization required', 'Confirm authorized bench use first.')
            return
        self.disconnect()
        try:
            port = self.port.get()
            serial = Serial(port)
        except (OSError, termios.error) as exc:
            messagebox.showerror('Connection failed', str(exc))
            return
        self.serial = serial
        self.model = Model('Live serial: ' + port)
        self.model.connected = True
        self.render()

    def disconnect(self):
        if self.serial:
            self.serial.close()
        self.serial = None
        self.model.connected = False
        self.render()

    def load(self, path):
        data = read_replay(path)
        self.disconnect()
        self.model = Model('Replay: ' + str(path))
        self.model.feed(data)
        self.model.finish()
        self.render()

    def replay(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load(path)
            except (OSError, ValueError) as exc:
                messagebox.showerror('Replay rejected', str(exc))

    def save(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                export(self.model, path)
            except OSError as exc:
                messagebox.showerror('Export failed', str(exc))

    def poll(self):
        if self.serial:
            try:
                data, hungup = self.serial.read()
                self.model.feed(data)
                if hungup:
                    self.disconnect()
            except (OSError, termios.error):
                self.disconnect()
        self.render()
        self.timer = self.root.after(25, self.poll)

    @staticmethod
    def text(widget, text):
        widget.configure(state='normal')
        widget.delete('1.0', 'end')
        widget.insert('1.0', text)
        widget.configure(state='disabled')

    def render(self):
        m = self.model
        age = m.age()
        live = 'LIVE' if m.connected else 'DISCONNECTED / REPLAY'
        freshness = 'no snapshot' if age is None else f'{age:.2f}s since accepted snapshot'
        if m.connected and (age is None or age > 1):
            live = 'STALE — no current evidence'
        self.status.set(f'{live} | {freshness} | {m.error or "not authenticated"}')
        if m.latest:
            v = m.latest
            findings = [name for bit, name in enumerate(FAULTS) if v['faults'] & (1 << bit)]
            text = (f'State: {STATES[v["state"]]}\nStarted: {v["started"]}\n'
                    f'Acknowledgments after quiet: {v["completed"]}\nAbandoned: {v["abandoned"]}\n'
                    f'Active age: {v["age"]} ms; quiet: {v["quiet"]} ms\n'
                    f'Admissions: {v["admissions"]}; blocked boundary events: {v["blocked"]}\n'
                    f'Early acknowledgments: {v["early"]}\n\nSticky findings:\n' + '\n'.join(findings or ['None observed']))
        else:
            text = 'No accepted status. Waiting for compatible RD1 policy and telemetry.'
        self.text(self.summary, text + '\n\nCompletion is not a safe-cycle claim.\nAuthorized low-voltage lab use only. Author: jayis1.')
        self.text(self.integrity, json.dumps(m.report(), indent=2))
        self.history.delete(*self.history.get_children())
        for row in m.rows:
            self.history.insert('', 'end', values=[row[k] for k in FIELDS])

    def close(self):
        self.root.after_cancel(self.timer)
        self.disconnect()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', type=Path, help='Validate a bounded recording without GUI')
    args = parser.parse_args()
    if args.check:
        m = Model('Replay: ' + str(args.check))
        try:
            m.feed(read_replay(args.check)); m.finish()
        except (ValueError, OSError) as exc:
            parser.exit(1, str(exc) + '\n')
        print(json.dumps(m.report(), indent=2))
        return 1 if m.error or m.errors else 0
    root = tk.Tk(); Window(root); root.mainloop()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
