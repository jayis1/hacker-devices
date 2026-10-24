#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Read-only Linux console for Grant Ledger; no external Python packages."""
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

FIELDS = ('time levels state synced age requests grants accepted revoked gaps missed '
          'dropped tx_fail no_credit orphan overlap ambiguous revoked_use expired').split()
FAULTS = ('no credit', 'orphan grant', 'overlap', 'ambiguous order', 'revoked use', 'expired')
STATES = ('IDLE', 'PENDING', 'CREDIT')

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
        self.boot = None
        self.events = deque(maxlen=256)
        self.buffer = bytearray()
        self.errors = 0
        self.sessions = 0
        self.received = None

    def fresh(self):
        return bool(self.connected and not self.replay and not self.restart and not self.incompatible
                    and self.received is not None and time.monotonic()-self.received < 1)

    def feed(self, data):
        # Bound memory even for a malicious sender which never sends LF.
        for byte in data:
            self.buffer.append(byte)
            if len(self.buffer) > 384:
                self.errors += 1
                self.buffer = self.buffer[-4:]
            if byte != 10:
                continue
            line = bytes(self.buffer).rstrip(b'\r\n')
            self.buffer.clear()
            # A timed-out partial UART transfer need not end in LF.
            marker = line.rfind(b'GL1,')
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
        if len(parts) < 3 or parts[0] != 'GL1':
            raise ValueError('version')
        kind = parts[1]
        sizes = {'B': 3, 'E': 4, 'S': 19}
        if kind not in sizes or len(parts)-2 != sizes[kind]:
            raise ValueError('field count')
        if any(not s or len(s)>10 or not s.isdecimal() for s in parts[2:]):
            raise ValueError('unsigned decimal required')
        values = [int(s) for s in parts[2:]]
        if any(v > 0xffffffff for v in values):
            raise ValueError('overflow')
        if kind == 'B':
            if values[1:] != [1000, 3]:
                self.incompatible = True
                raise ValueError('unsupported policy')
            self.incompatible = False
            self.boot = values
            self.status = None
            self.received = None
            self.events.clear()
            self.restart = False
            self.sessions += 1
        elif kind == 'S':
            if values[1]>15 or values[2]>2 or values[3]>1 or values[4]>1000:
                raise ValueError('status bounds')
            if self.restart or self.incompatible:
                return
            if self.status and ((values[0]-self.status[0]) & 0xffffffff) >= 0x80000000:
                self.restart = True
                return
            self.status = values
            self.received = time.monotonic()
        else:
            _, _, event_kind, value = values
            bounds = {1:15, 2:5, 3:2, 4:0xffffffff, 5:0xffffffff, 6:1}
            if event_kind not in bounds or value>bounds[event_kind]:
                raise ValueError('event bounds')
            if not self.restart and not self.incompatible:
                self.events.append(values)

    def snapshot(self):
        age = None if self.received is None else max(0,time.monotonic()-self.received)
        return dict(author='jayis1', source=self.source, replay=self.replay,
                    connected=self.connected, fresh=self.fresh(), receipt_age_seconds=age,
                    restart_suspected=self.restart, incompatible_policy=self.incompatible,
                    sessions=self.sessions, boot=self.boot,
                    status=dict(zip(FIELDS,self.status)) if self.status else None,
                    errors=self.errors, events=list(self.events))

    def export(self, path):
        # Exclusive creation protects previous evidence and refuses symlinks.
        fd = os.open(path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(self.snapshot(), stream, indent=2)
            stream.write('\n')

class Port:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY|os.O_NOCTTY|os.O_NONBLOCK)
        try:
            tty.setraw(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[4] = settings[5] = termios.B115200
            settings[2] |= termios.CLOCAL|termios.CREAD
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
        except Exception:
            os.close(self.fd)
            raise

    def poll(self):
        if not select.select([self.fd], [], [], 0)[0]:
            return None
        return os.read(self.fd, 4096)

    def close(self):
        os.close(self.fd)

def screen_lines(model, page):
    state = 'LIVE' if model.fresh() else 'NOT LIVE'
    lines = [f'Grant Ledger / jayis1 / {state}',
             '1 Overview | 2 Events | 3 Health | e Export | q Quit',
             f'Source: {model.source}', '']
    if model.restart:
        lines += ['RESTART SUSPECTED: reconnect or await boot record; evidence frozen.']
    if model.incompatible:
        lines += ['INCOMPATIBLE POLICY: status frozen until a supported boot record.']
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
        lines += ['TRANSACTION OVERVIEW']
        if model.status:
            s = dict(zip(FIELDS,model.status))
            lines += [f'State: {STATES[s["state"]]}  synchronized: {s["synced"]}',
                      'Inputs: '+' '.join(f'{name}={(s["levels"]>>i)&1}' for i,name in
                                           enumerate(('REQUEST','GRANT','USE','REVOKE'))),
                      f'Requests: {s["requests"]} Grants: {s["grants"]} Accepted: {s["accepted"]}',
                      f'Window age: {s["age"]} ms  Revocations: {s["revoked"]}']
            lines += [f'{name}: {value}' for name,value in zip(FAULTS,model.status[13:])]
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
    parser.add_argument('--export', type=Path, default=Path('grant-evidence.json'))
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
    except (OSError,ValueError,curses.error) as exc:
        parser.exit(1,f'Grant Ledger: {exc}\n')
    finally:
        if port is not None:
            port.close()

if __name__ == '__main__':
    main()
