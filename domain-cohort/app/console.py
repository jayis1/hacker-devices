#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Read-only Linux desktop for DC1 evidence. No third-party Python dependencies."""
import binascii
from collections import deque
import json
import os
import re
import select
import stat
import termios
import time
import tkinter as tk
from tkinter import filedialog, ttk

FILE_LIMIT = 2 * 1024 * 1024
from contract import FIELDS, COUNTERS, FAULTS, validate, transition, queue

def frame(kind, values):
    """Also used by tests to create checksum-valid impossible records."""
    body = ('DC1,' + kind + ''.join(',' + str(v) for v in values)).encode('ascii')
    return body + f'*{binascii.crc_hqx(body, 0xffff):04X}\n'.encode('ascii')

class Model:
    def __init__(self, source, origin):
        self.source, self.origin = str(source), origin
        self.connected = False
        self.policy = False
        self.frozen = ''
        self.errors = 0
        self.buffer = bytearray()
        self.latest = None
        self.history = deque(maxlen=256)
        self.received = None
        self.first_host = None
        self.device_elapsed = 0
        self.progress = False
        self.reset = None
        self.later_reset = None

    def live(self):
        return bool(self.connected and self.policy and self.progress and not self.frozen and self.latest and
                    self.received is not None and time.monotonic() - self.received <= 1)

    def accept(self, line):
        if not re.fullmatch(rb'DC1,[BPS](?:,[0-9]{1,10})+\*[0-9A-F]{4}', line):
            raise ValueError('frame syntax')
        body, checksum = line.split(b'*')
        if binascii.crc_hqx(body, 0xffff) != int(checksum, 16):
            raise ValueError('CRC')
        pieces = body.decode('ascii').split(',')
        values = [int(v) for v in pieces[2:]]
        if any(v > 0xffffffff for v in values):
            raise ValueError('integer overflow')
        if pieces[1] == 'B':
            if len(values) != 1:
                raise ValueError('boot fields')
            if self.latest:
                self.frozen = 'Boot after evidence; start a new session'
                self.later_reset = values[0]
            else:
                self.reset = values[0]
            return
        if pieces[1] == 'P':
            self.policy = values == [3, 1, 5, 2]
            if not self.policy:
                self.frozen = 'Unsupported policy; start a compatible session'
            return
        if len(values) != len(FIELDS):
            raise ValueError('status fields')
        s = dict(zip(FIELDS, values))
        validate(s)
        if not self.policy or self.frozen:
            return
        host = time.monotonic()
        if self.latest:
            previous = self.latest
            delta = (s['now'] - previous['now']) & 0xffffffff
            if not delta:
                if s != previous:
                    self.frozen = 'Contradictory duplicate timestamp'
                return
            if delta >= 0x80000000 or any(s[k] < previous[k] for k in COUNTERS):
                self.frozen = 'Clock or counter rollback; possible reset'
                return
            if s['faults'] & previous['faults'] != previous['faults']:
                self.frozen = 'Sticky fault disappeared'
                return
            if previous['halted'] and any(s[k] != previous[k] for k in FIELDS[1:-1]):
                self.frozen = 'Halted evidence changed'
                return
            for key in COUNTERS:
                if key in ('missed', 'tx_fail'):
                    continue
                limit = 2 * delta if key in ('attempts', 'rejected') else delta
                if s[key] - previous[key] > limit:
                    self.frozen = 'Counter increment exceeds elapsed device time'
                    return
            self.frozen = transition(previous, s, delta)
            if self.frozen:
                return
            if self.origin == 'live':
                assert self.received is not None and self.first_host is not None
                if host - self.received > 1:
                    self.frozen = 'Reception pause; reconnect to discard backlog'
                    return
                self.device_elapsed += delta / 1000
                elapsed = host - self.first_host
                # Drift allowance is capped: a slow reader must not earn an
                # unbounded stale-data budget simply by staying connected.
                if abs(self.device_elapsed - elapsed) > .5 + min(.5, .05 * elapsed):
                    self.frozen = 'Host/device progress diverged; reconnect'
                    return
            self.progress = True
        else:
            self.first_host = host
        self.latest = s
        self.received = host
        self.history.append(s)

    def feed(self, data):
        # Bound before processing; callers supply reads no larger than 4096 bytes.
        for ch in data:
            self.buffer.append(ch)
            if self.buffer.endswith(b'DC1,') and len(self.buffer) > 4:
                self.errors += 1
                self.buffer[:] = b'DC1,'
            if ch == 10:
                try:
                    self.accept(bytes(self.buffer[:-1]))
                except ValueError:
                    self.errors += 1
                self.buffer.clear()
            elif len(self.buffer) > 768:
                self.errors += 1
                self.buffer.clear()

    def finish(self):
        if self.buffer:
            self.errors += 1
            self.buffer.clear()
        if self.latest is None:
            raise ValueError('No compatible status in replay')

    def report(self):
        return dict(author='jayis1', source=self.source, origin=self.origin,
                    connected=self.connected, live=self.live(), policy=self.policy,
                    frozen=self.frozen, errors=self.errors, reset=self.reset, later_reset=self.later_reset,
                    age=None if self.received is None else time.monotonic()-self.received,
                    latest=self.latest, history=list(self.history))

    def export(self, path):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600)
        with os.fdopen(fd, 'w') as output:
            json.dump(self.report(), output, indent=2)
            output.write('\n')

def read_replay(path):
    fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > FILE_LIMIT:
            raise ValueError('Replay must be a regular file of at most two MiB')
        with os.fdopen(fd, 'rb', closefd=False) as stream:
            data = stream.read(FILE_LIMIT + 1)
        if len(data) > FILE_LIMIT:
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
            settings = termios.tcgetattr(self.fd)
            settings[0] = settings[1] = settings[3] = 0
            settings[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            settings[4] = settings[5] = termios.B115200
            settings[6][termios.VMIN] = settings[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
            self.poller = select.poll()
            self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)
        except Exception:
            self.close()
            raise

    def close(self):
        if self.fd is not None:
            if self.old is not None:
                try:
                    termios.tcsetattr(self.fd, termios.TCSANOW, self.old)
                except (OSError, termios.error):
                    pass
            os.close(self.fd)
            self.fd = None

class Console:
    def __init__(self, root):
        self.root = root
        self.port = None
        self.model = Model('', 'none')
        self.message = ''
        root.title('Domain Cohort — jayis1')
        root.geometry('940x650')
        self.path = tk.StringVar(value='/dev/ttyACM0')
        bar = ttk.Frame(root); bar.pack(fill='x')
        ttk.Entry(bar, textvariable=self.path, width=35).pack(side='left')
        for title, fn in [('New live', self.connect), ('Disconnect', self.disconnect),
                          ('Replay', self.replay), ('Export JSON', self.export)]:
            ttk.Button(bar, text=title, command=fn).pack(side='left')
        self.tabs = ttk.Notebook(root); self.tabs.pack(fill='both', expand=True)
        self.summary = self.page('Ledger')
        self.history = self.page('History')
        self.integrity = self.page('Integrity')
        self.job = root.after(20, self.poll)
        root.protocol('WM_DELETE_WINDOW', self.quit)

    def page(self, title):
        text = tk.Text(self.tabs, wrap='word')
        self.tabs.add(text, text=title)
        return text

    def connect(self):
        if self.port:
            self.message = 'Disconnect before opening another serial session'
            return
        try:
            port = Serial(self.path.get())
        except (OSError, termios.error) as error:
            self.message = str(error)
            return
        self.port = port
        self.model = Model(self.path.get(), 'live')
        self.model.connected = True
        self.message = ''

    def disconnect(self):
        if self.port:
            self.port.close()
            self.port = None
        # A partial frame at hangup is lost evidence, not a clean stream.
        if self.model.buffer:
            self.model.errors += 1
            self.model.buffer.clear()
        self.model.connected = False

    def load(self, path):
        model = Model(path, 'replay')
        model.feed(read_replay(path)); model.finish()
        self.disconnect()
        self.model = model
        self.message = ''
        self.draw()

    def replay(self):
        path = filedialog.askopenfilename()
        if path:
            try:
                self.load(path)
            except (OSError, ValueError) as error:
                self.message = str(error)

    def export(self):
        path = filedialog.asksaveasfilename(defaultextension='.json')
        if path:
            try:
                self.model.export(path)
                self.message = 'Saved private snapshot: ' + path
            except OSError as error:
                self.message = str(error)

    def draw(self):
        s = self.model.latest
        ledger = 'No compatible measurements yet'
        if s:
            ledger = '\n'.join(f'{key}: {value}' for key, value in s.items())
            ledger += '\n\nFindings: ' + ', '.join(name for i, name in enumerate(FAULTS) if s['faults'] & (1 << i))
            ledger += '\n\nPhase: ' + ['WAIT', 'TRACK'][s['phase']]
            ledger += '\nActive (slot, domain): ' + str(queue(s))
            ledger += '\nRelease must name the bound domain; parallel slots must share a domain.'
            if s['halted']: ledger += ' / HALTED'
            ledger += '\nCounts describe qualified observations, not exact electrical milliseconds.'
        for widget, content in [(self.summary, ledger),
                                (self.history, '\n'.join(str(s) for s in self.model.history)),
                                (self.integrity, json.dumps({k:v for k,v in self.model.report().items()
                                                            if k not in ('history','latest')}, indent=2)+'\n'+self.message)]:
            widget.configure(state='normal'); widget.delete('1.0','end')
            widget.insert('end', content); widget.configure(state='disabled')

    def poll(self):
        if self.port:
            try:
                for _, flags in self.port.poller.poll(0):
                    if flags & select.POLLIN:
                        assert self.port.fd is not None
                        self.model.feed(os.read(self.port.fd, 4096))
                    if flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL):
                        self.disconnect()
                        self.message = 'Serial disconnected; retained evidence is not live'
                        break
            except OSError as error:
                self.disconnect(); self.message = str(error)
        self.draw()
        self.job = self.root.after(20, self.poll)

    def close(self):
        self.root.after_cancel(self.job)
        self.disconnect()

    def quit(self):
        self.close(); self.root.destroy()

if __name__ == '__main__':
    root = tk.Tk()
    Console(root)
    root.mainloop()
