#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Read-only Linux console for Coincidence Guard; standard library only."""
import argparse
from collections import deque
import curses
import json
import os
from pathlib import Path
import select
import termios
import time
import tty

FIELDS = ('time levels valid state age skew_ms pairs rejected unsafe expired '
          'revoked gaps missed dropped tx_fail').split()
STATES = ('UNKNOWN', 'READY', 'WAIT_A', 'WAIT_B', 'PAIRED', 'LOCKED')

def crc(data):
    result = 0xffff
    for byte in data:
        result ^= byte << 8
        for _ in range(8):
            result = ((result << 1) ^ (0x1021 if result & 0x8000 else 0)) & 0xffff
    return result

class Model:
    def __init__(self, source, replay=False):
        self.source = source
        self.replay = replay
        self.connected = False
        self.restart = False
        self.incompatible = False
        self.status = None
        self.policy_known = False
        self.boot = None
        self.events = deque(maxlen=256)
        self.buffer = bytearray()
        self.errors = 0
        self.sessions = 0
        self.received = None

    def fresh(self):
        return bool(self.connected and self.policy_known and not self.replay and not self.restart
                    and not self.incompatible and self.received is not None
                    and time.monotonic()-self.received < 1)

    def feed(self, data):
        # Bound memory even when a malicious sender never sends LF.
        for byte in data:
            self.buffer.append(byte)
            if len(self.buffer) > 384:
                self.errors += 1
                self.buffer = self.buffer[-4:]
            if byte != 10:
                continue
            line = bytes(self.buffer).rstrip(b'\r\n')
            self.buffer.clear()
            marker = line.rfind(b'CG1,')
            if marker > 0:
                self.errors += 1
                line = line[marker:]
            try:
                self.record(line)
            except (ValueError, UnicodeError):
                self.errors += 1

    def record(self, line):
        body, checksum = line.rsplit(b'*', 1)
        if len(checksum) != 4 or any(c not in b'0123456789ABCDEF' for c in checksum):
            raise ValueError('checksum syntax')
        if crc(body) != int(checksum, 16):
            raise ValueError('checksum mismatch')
        parts = body.decode('ascii').split(',')
        if len(parts) < 3 or parts[0] != 'CG1':
            raise ValueError('version')
        kind = parts[1]
        sizes = {'B': 3, 'P': 2, 'E': 4, 'S': 15}
        if kind not in sizes or len(parts)-2 != sizes[kind]:
            raise ValueError('field count')
        if any(not s or len(s)>10 or not s.isdecimal() for s in parts[2:]):
            raise ValueError('unsigned decimal required')
        values = [int(s) for s in parts[2:]]
        if any(v > 0xffffffff for v in values):
            raise ValueError('overflow')
        if kind in ('B','P'):
            policy = values[1:] if kind == 'B' else values
            if policy != [250, 3]:
                self.incompatible = True
                self.policy_known = False
                raise ValueError('unsupported policy')
            self.incompatible = False
            self.policy_known = True
            if kind == 'P':
                return  # Repeated policy is not a reset or a new session.
            self.boot = values
            self.status = None
            self.received = None
            self.events.clear()
            self.restart = False
            self.sessions += 1
        elif kind == 'S':
            if any(values[i] > bound for i,bound in [(1,15),(2,1),(3,5)]):
                raise ValueError('status bounds')
            if self.restart or self.incompatible:
                return
            if self.status and (((values[0]-self.status[0]) & 0xffffffff) >= 0x80000000
                                or any(values[i] < self.status[i] for i in range(6,15))):
                self.restart = True
                return
            self.status = values
            self.received = time.monotonic()
        else:
            _, _, event_kind, value = values
            bounds = {1:15, 2:5, 3:0xffffffff, 4:0xffffffff,
                      5:0xffffffff, 6:0xffffffff, 7:0xffffffff, 8:0xffffffff}
            if event_kind not in bounds or value>bounds[event_kind]:
                raise ValueError('event bounds')
            if not self.restart and not self.incompatible:
                self.events.append(values)

    def snapshot(self):
        age = None if self.received is None else max(0,time.monotonic()-self.received)
        return dict(author='jayis1', source=self.source, replay=self.replay,
                    connected=self.connected, fresh=self.fresh(), receipt_age_seconds=age,
                    restart_suspected=self.restart, incompatible_policy=self.incompatible,
                    policy_known=self.policy_known, sessions=self.sessions, boot=self.boot,
                    status=dict(zip(FIELDS,self.status)) if self.status else None,
                    errors=self.errors, events=list(self.events))

    def export(self, path):
        # Exclusive creation protects previous evidence and refuses final symlinks.
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(self.snapshot(), stream, indent=2)
            stream.write('\n')

class Port:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY|os.O_NOCTTY|os.O_NONBLOCK)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            tty.setraw(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[4] = settings[5] = termios.B115200
            settings[2] |= termios.CLOCAL|termios.CREAD
            settings[2] &= ~termios.CRTSCTS
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
        except Exception:
            self.close()
            raise

    def poll(self):
        if self.fd is None:
            return b''
        if not select.select([self.fd], [], [], 0)[0]:
            return None
        return os.read(self.fd, 4096)

    def close(self):
        if self.fd is None:
            return
        try:
            if self.original is not None:
                termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
        except (OSError, termios.error):
            pass  # Unplug may make restoration impossible.
        finally:
            os.close(self.fd)
            self.fd = None

def screen_lines(model, page):
    state = 'LIVE' if model.fresh() else 'NOT LIVE'
    lines = [f'Coincidence Guard / jayis1 / {state}',
             '1 Overview | 2 Events | 3 Health | e Export | q Quit',
             f'Source: {model.source}', '']
    if model.restart:
        lines += ['RESTART SUSPECTED: reconnect or await boot record; evidence frozen.']
    if model.incompatible:
        lines += ['INCOMPATIBLE POLICY: status frozen until supported policy arrives.']
    if not model.policy_known:
        lines += ['Waiting for compatible policy announcement; not live.']
    if page == 2:
        lines += ['EVENT HISTORY: sequence / device-ms / kind / value']
        lines += [' / '.join(map(str,event)) for event in list(model.events)[-16:]]
    elif page == 3:
        lines += ['HEALTH', f'Parse errors: {model.errors}', f'Boot sessions: {model.sessions}',
                  f'Connected: {model.connected}  Replay: {model.replay}']
        if model.status:
            lines += [f'{name}: {value}' for name,value in zip(FIELDS,model.status) if name in
                      ('gaps','missed','dropped','tx_fail')]
    else:
        lines += ['BILATERAL CONSENT (not a safety controller or identity proof)']
        if model.status:
            s = dict(zip(FIELDS,model.status))
            lines += [f'State: {STATES[s["state"]]}  qualified: {s["valid"]}',
                      'Inputs: '+' '.join(f'{name}={(s["levels"]>>i)&1}' for i,name in
                                           enumerate(('A','B','PERMIT','INHIBIT'))),
                      f'State age: {s["age"]} ms; last pair skew: {s["skew_ms"]} ms']
            lines += [f'{name}: {s[name]}' for name in FIELDS[6:11]]
        else:
            lines += ['Waiting for a valid status snapshot.']
    return lines

def console(window, model, port, export):
    window.timeout(50)
    page = 1
    notice = ''
    while True:
        if port is not None and model.connected:
            try:
                data = port.poll()
                if data == b'':
                    model.connected = False
                    notice = 'Serial disconnected; restart application to reconnect.'
                elif data:
                    model.feed(data)
            except OSError as exc:
                model.connected = False
                notice = f'Serial disconnected: {exc.strerror}'
        window.erase()
        height, width = window.getmaxyx()
        for row,line in enumerate(screen_lines(model,page)+['',notice]):
            if row >= height-1:
                break
            try:
                window.addnstr(row,0,line,max(0,width-1))
            except curses.error:
                pass
        window.refresh()
        key = window.getch()
        if key == ord('q'):
            return
        if key in (ord('1'),ord('2'),ord('3')):
            page = key-ord('0')
        if key == ord('e'):
            try:
                model.export(export)
                notice = 'Export saved (private JSON).'
            except OSError as exc:
                notice = f'Export refused: {exc.strerror}'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--port')
    source.add_argument('--replay', type=Path)
    parser.add_argument('--export', type=Path, default=Path('consent-evidence.json'))
    parser.add_argument('--headless', action='store_true', help='Replay only; export and exit')
    args = parser.parse_args()
    if args.headless and not args.replay:
        parser.error('--headless requires --replay')
    model = Model(str(args.replay or args.port), replay=bool(args.replay))
    port = None
    try:
        if args.replay:
            with args.replay.open('rb') as stream:
                data = stream.read(16*1024*1024+1)
            if len(data)>16*1024*1024:
                raise ValueError('replay exceeds 16 MiB')
            model.feed(data)
        else:
            port = Port(args.port)
            model.connected = True
        if args.headless:
            model.export(args.export)
        else:
            curses.wrapper(console,model,port,args.export)
    except (OSError,ValueError,curses.error,termios.error) as exc:
        parser.exit(1,f'Coincidence Guard: {exc}\n')
    finally:
        if port is not None:
            port.close()

if __name__ == '__main__':
    main()
