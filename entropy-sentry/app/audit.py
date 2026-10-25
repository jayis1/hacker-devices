#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Entropy Sentry read-only Linux console; no external Python dependencies."""
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

FIELDS = ('time observed qualified faults window_used last_matches max_run samples windows '
          'failures unsafe_episodes unsafe_ms unannounced_ms gaps missed stalls dropped tx_fail').split()
POLICY = [32, 64, 49, 10, 100, 2]


def crc(data):
    value = 0xffff
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xffff
    return value


class Model:
    def __init__(self, source, replay=False):
        self.source = source
        self.replay = replay
        self.connected = False
        self.policy_known = False
        self.incompatible = False
        self.restart = False
        self.status = None
        self.received = None
        self.buffer = bytearray()
        self.errors = 0
        self.sessions = 0
        self.boot_flags = None
        self.events = deque(maxlen=256)

    def fresh(self):
        return bool(self.connected and not self.replay and self.policy_known
                    and not self.restart and not self.incompatible
                    and self.received is not None and time.monotonic()-self.received < 1)

    def feed(self, data):
        for byte in data:
            self.buffer.append(byte)
            if len(self.buffer) > 384:
                self.errors += 1
                self.buffer = self.buffer[-4:]
            if byte != 10:
                continue
            line = bytes(self.buffer).rstrip(b'\r\n')
            self.buffer.clear()
            marker = line.rfind(b'ES1,')
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
        if len(parts) < 3 or parts[0] != 'ES1':
            raise ValueError('protocol version')
        kind = parts[1]
        if kind not in ('B', 'P', 'S', 'E'):
            raise ValueError('record type')
        if len(parts)-2 != {'B': 1, 'P': 6, 'S': 18, 'E': 4}[kind]:
            raise ValueError('field count')
        if any(not s or len(s) > 10 or not s.isdecimal() for s in parts[2:]):
            raise ValueError('unsigned decimal required')
        values = [int(s) for s in parts[2:]]
        if any(v > 0xffffffff for v in values):
            raise ValueError('numeric overflow')
        if kind == 'P':
            self.policy_known = values == POLICY
            self.incompatible = not self.policy_known
            if self.incompatible:
                raise ValueError('incompatible policy')
            return
        if kind == 'B':
            # Preserve old evidence on an in-session boot. Explicit reconnect is required.
            if self.status is not None:
                self.restart = True
            self.boot_flags = values[0]
            self.sessions += 1
            return
        if kind == 'E':
            if values[2] not in (1, 2, 3, 4, 5):
                raise ValueError('event type')
            limits = {1: 15, 2: 64, 3: 3, 4: 0xffffffff, 5: 0xffffffff}
            if values[3] > limits[values[2]]:
                raise ValueError('event value')
            if not self.restart and not self.incompatible:
                self.events.append(values)
            return
        if any(values[i] > bound for i, bound in [(1,3), (2,1), (3,15), (4,63), (5,64)]):
            raise ValueError('status bounds')
        if values[2] and values[3]:
            raise ValueError('qualified despite sticky fault')
        status = dict(zip(FIELDS, values))
        if self.restart or self.incompatible:
            return
        if self.status is not None:
            elapsed = (status['time']-self.status['time']) & 0xffffffff
            lost_fault = self.status['faults'] & ~status['faults']
            if (elapsed >= 0x80000000 or lost_fault
                    or any(status[k] < self.status[k] for k in FIELDS[6:])):
                self.restart = True
                return
            if elapsed == 0:
                if status != self.status:
                    self.restart = True
                return  # Duplicates must not make a frozen device appear fresh.
        self.status = status
        self.received = time.monotonic()

    def snapshot(self):
        return dict(author='jayis1', source=self.source, replay=self.replay,
                    connected=self.connected, fresh=self.fresh(),
                    receipt_age_seconds=None if self.received is None else time.monotonic()-self.received,
                    policy_known=self.policy_known, incompatible_policy=self.incompatible,
                    restart_suspected=self.restart, boot_flags=self.boot_flags,
                    sessions=self.sessions, parser_errors=self.errors,
                    status=self.status, events=list(self.events))

    def export(self, path):
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(self.snapshot(), stream, indent=2)
            stream.write('\n')


class Port:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
        self.original = None
        try:
            self.original = termios.tcgetattr(self.fd)
            tty.setraw(self.fd)
            settings = termios.tcgetattr(self.fd)
            settings[4] = settings[5] = termios.B115200
            settings[2] |= termios.CLOCAL | termios.CREAD
            settings[2] &= ~termios.CRTSCTS
            termios.tcsetattr(self.fd, termios.TCSANOW, settings)
            termios.tcflush(self.fd, termios.TCIFLUSH)
        except Exception:
            self.close()
            raise
        self.poller = select.poll()
        self.poller.register(self.fd, select.POLLIN | select.POLLHUP | select.POLLERR)

    def poll(self):
        if self.fd is None:
            return b''
        events = self.poller.poll(0)
        if not events:
            return None
        if any(flags & (select.POLLHUP | select.POLLERR | select.POLLNVAL) for _,flags in events):
            return b''
        try:
            return os.read(self.fd, 4096)
        except BlockingIOError:
            return None

    def close(self):
        if self.fd is None:
            return
        try:
            if self.original is not None:
                termios.tcsetattr(self.fd, termios.TCSANOW, self.original)
        except (OSError, termios.error):
            pass
        finally:
            os.close(self.fd)
            self.fd = None


def screen_lines(model, page):
    lines = [f'Entropy Sentry / jayis1 / {"LIVE" if model.fresh() else "NOT LIVE"}',
             '1 Health tests | 2 Events | 3 Integrity | e Export | q Quit',
             f'Source: {model.source}', 'Observation only; passing is NOT proof of entropy.']
    if model.restart:
        lines.append('RESTART SUSPECTED: evidence frozen; reconnect explicitly.')
    if not model.policy_known:
        lines.append('Waiting for supported policy; incompatible or unknown.')
    if page == 2:
        lines.append('EVENTS: sequence / device-ms / kind / value')
        lines.extend(' / '.join(map(str,e)) for e in list(model.events)[-16:])
    elif page == 3:
        lines += ['INTEGRITY', f'Parser errors: {model.errors}',
                  f'Connected: {model.connected} Replay: {model.replay} Boot records: {model.sessions}']
        if model.status:
            lines.extend(f'{k}: {model.status[k]}' for k in FIELDS[13:])
    elif model.status:
        s = model.status
        lines += ['HEALTH TESTS', f'Qualified window observed: {bool(s["qualified"])}',
                  f'Sticky faults: {s["faults"]} (1 repeat, 2 proportion, 4 setup, 8 rate)',
                  f'ALARM: {s["observed"]&1} RELEASE: {(s["observed"]>>1)&1}']
        lines.extend(f'{k}: {s[k]}' for k in FIELDS[4:13])
    else:
        lines.append('Waiting for valid telemetry.')
    return lines


def console(window, model, port, export):
    window.timeout(50)
    page, notice = 1, ''
    while True:
        if port is not None and model.connected:
            try:
                data = port.poll()
                if data == b'':
                    model.connected = False
                    notice = 'Disconnected; restart application to reconnect.'
                elif data:
                    model.feed(data)
            except OSError:
                model.connected = False
                notice = 'Serial read failed; evidence retained.'
        window.erase()
        height, width = window.getmaxyx()
        for row, line in enumerate(screen_lines(model, page)+['', notice]):
            if row >= height-1:
                break
            try:
                window.addnstr(row, 0, line, max(0, width-1))
            except curses.error:
                pass
        window.refresh()
        key = window.getch()
        if key == ord('q'):
            return
        if key in (ord('1'), ord('2'), ord('3')):
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
    parser.add_argument('--export', type=Path, default=Path('entropy-evidence.json'))
    parser.add_argument('--headless', action='store_true')
    args = parser.parse_args()
    if args.headless and not args.replay:
        parser.error('--headless requires --replay')
    model = Model(str(args.replay or args.port), replay=bool(args.replay))
    port = None
    try:
        if args.replay:
            with args.replay.open('rb') as stream:
                data = stream.read(16*1024*1024+1)
            if len(data) > 16*1024*1024:
                raise ValueError('Replay exceeds 16 MiB')
            model.feed(data)
        else:
            port = Port(args.port)
            model.connected = True
        if args.headless:
            model.export(args.export)
        else:
            curses.wrapper(console, model, port, args.export)
    except (OSError, ValueError, termios.error) as exc:
        parser.exit(1, f'Entropy Sentry: {exc}\n')
    finally:
        if port:
            port.close()


if __name__ == '__main__':
    main()
