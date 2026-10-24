#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Lease Watch Linux console. Standard library only; never writes to serial."""
import argparse
from collections import deque
import curses
import json
import os
from pathlib import Path
import select
import stat
import termios
import time

AUTHOR = 'jayis1'
FIELDS = ('time levels alarms age heartbeats last_period min_period max_period '
          'gaps missed dropped tx_failures max_enabled_stale no_heartbeat_entries '
          'expired_entries reset_entries not_ready_entries uncertain_entries').split()
ALARMS = ['NO_HEARTBEAT', 'EXPIRED', 'RESET_ENABLED', 'NOT_READY', 'UNCERTAIN']


def crc(raw):
    value = 0xFFFF
    for byte in raw:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xFFFF
    return value


def parse(raw):
    if len(raw) > 383 or not raw.endswith(b'\n'):
        raise ValueError('frame size or termination')
    try:
        body, check = raw[:-1].rsplit(b'*', 1)
        if len(check) != 4 or any(c not in b'0123456789ABCDEF' for c in check):
            raise ValueError('CRC encoding')
        if crc(body) != int(check, 16):
            raise ValueError('CRC mismatch')
        fields = body.decode('ascii').split(',')
        if fields[0] != 'LW1' or fields[1] not in ('B', 'S', 'E'):
            raise ValueError('protocol version or record')
        kind = fields[1]
        if len(fields) != {'B': 5, 'S': 20, 'E': 6}[kind]:
            raise ValueError('field count')
        if any(not x or len(x) > 10 or not x.isascii() or not x.isdecimal() for x in fields[2:]):
            raise ValueError('unsigned decimal expected')
        values = [int(x) for x in fields[2:]]
        if any(x > 0xFFFFFFFF for x in values):
            raise ValueError('integer range')
        if kind == 'B' and values[1:] != [250, 3]:
            raise ValueError('unsupported policy')
        if kind == 'S' and (values[1] > 15 or values[2] > 31):
            raise ValueError('status mask')
        if kind == 'E':
            if values[2] not in (1, 2, 3, 4):
                raise ValueError('event kind')
            if values[2] == 1 and values[3] > 15:
                raise ValueError('event level')
            if values[2] == 2 and values[3] > 31:
                raise ValueError('event alarm')
        return kind, values
    except (UnicodeError, IndexError) as error:
        raise ValueError('malformed frame') from error


class Framer:
    def __init__(self):
        self.buffer = bytearray()
        self.discard = False
        self.rejected = 0

    def feed(self, data):
        frames = []
        for byte in data:
            if self.discard:
                if byte == 10:
                    self.discard = False
                continue
            self.buffer.append(byte)
            if len(self.buffer) > 383:
                self.rejected += 1
                self.buffer.clear()
                self.discard = byte != 10
            elif byte == 10:
                frames.append(bytes(self.buffer))
                self.buffer.clear()
        return frames


class Model:
    def __init__(self):
        self.status = None
        self.received = None
        self.events = deque(maxlen=256)
        self.sessions = 1
        self.connected = False
        self.source = 'none'
        self.errors = 0
        self.boot = None
        self.message = ''

    def accept(self, record, now):
        kind, values = record
        if kind == 'B':
            if self.boot is not None or self.status is not None:
                self.sessions += 1
            self.boot = values
            self.status = None
            self.received = None
            self.events.clear()
        elif kind == 'S':
            if self.status is not None:
                delta = (values[0] - self.status['time']) & 0xFFFFFFFF
                if delta >= 0x80000000:
                    self.sessions += 1
                    self.events.clear()
                    self.message = 'Inferred restart from status time rollback'
            self.status = dict(zip(FIELDS, values))
            self.received = now
        else:
            self.events.append(dict(zip(('sequence', 'time', 'kind', 'value'), values)))

    def fresh(self, now):
        return (self.connected and self.received is not None
                and now - self.received <= 1.0)

    def export(self, path, now):
        data = {'author': AUTHOR, 'protocol': 'LW1', 'source': self.source,
                'connected': self.connected, 'fresh': self.fresh(now),
                'status_age_seconds': None if self.received is None else now - self.received,
                'sessions': self.sessions, 'status': self.status,
                'events': list(self.events), 'parse_errors': self.errors,
                'boot': self.boot, 'message': self.message}
        # O_EXCL prevents accidental evidence overwrite and symlink following.
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'w') as stream:
            json.dump(data, stream, indent=2)
            stream.write('\n')


def serial_open(path):
    fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        if not stat.S_ISCHR(os.fstat(fd).st_mode) or not os.isatty(fd):
            raise ValueError('serial device must be a TTY')
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        return fd
    except Exception:
        os.close(fd)
        raise


def ingest(model, framer, data):
    before = framer.rejected
    for raw in framer.feed(data):
        try:
            model.accept(parse(raw), time.monotonic())
        except ValueError:
            model.errors += 1
    model.errors += framer.rejected - before


def screen_lines(model, page):
    now = time.monotonic()
    result = ['Lease Watch / jayis1',
              '1 Overview  2 Events  3 Health  e Export  q Quit',
              f'Source: {model.source}  Connection: {model.connected}',
              f'Status: {"FRESH" if model.fresh(now) else "STALE / NOT LIVE"}  Session: {model.sessions}']
    s = model.status
    if page == 1:
        result.append('OVERVIEW - observed enable / heartbeat contract')
        if s:
            result.extend([f'Levels HB ENABLE RESET_N READY: {s["levels"]:04b} (bit 0 = HB)',
                           'Alarms: ' + (' | '.join(name for i, name in enumerate(ALARMS) if s['alarms'] & (1 << i)) or 'none'),
                           f'Heartbeat age ms: {s["age"]}  Edges: {s["heartbeats"]}',
                           f'Periods last/min/max: {s["last_period"]}/{s["min_period"]}/{s["max_period"]}',
                           f'Max enabled-stale duration ms: {s["max_enabled_stale"]}'])
        else:
            result.append('Waiting for a valid status frame')
    elif page == 2:
        result.append('EVENTS - last 256, newest first; not a complete capture')
        for e in reversed(model.events):
            result.append(f'{e["sequence"]:10}  {e["time"]:10} ms  kind={e["kind"]} value={e["value"]}')
    else:
        result.append('HEALTH - loss counters invalidate completeness claims')
        result.append(f'Invalid frames: {model.errors}; boot metadata: {model.boot}')
        if s:
            for key in FIELDS[8:]:
                result.append(f'{key}: {s[key]}')
    result.append(model.message)
    return result


def run_ui(window, model, fd, output):
    window.nodelay(True)
    page = 1
    framer = Framer()
    while True:
        if model.connected:
            try:
                readable, _, _ = select.select([fd], [], [], 0)
                if readable:
                    data = os.read(fd, 4096)
                    if data:
                        ingest(model, framer, data)
                    else:
                        model.connected = False
                        model.message = 'Serial disconnected; reconnect by restarting app'
            except OSError as error:
                model.connected = False
                model.message = f'Serial unavailable: {error}'
        window.erase()
        height, width = window.getmaxyx()
        for row, text in enumerate(screen_lines(model, page)[:max(0, height - 1)]):
            try:
                window.addnstr(row, 0, text, max(0, width - 1))
            except curses.error:
                pass
        window.refresh()
        key = window.getch()
        if key == ord('q'):
            return
        if key in (ord('1'), ord('2'), ord('3')):
            page = key - ord('0')
        if key == ord('e'):
            try:
                model.export(output, time.monotonic())
                model.message = f'Exported {output}'
            except OSError as error:
                model.message = f'Export failed: {error}'
        time.sleep(0.05)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--port')
    group.add_argument('--replay', type=Path)
    parser.add_argument('--export', type=Path, default=Path('lease-report.json'))
    parser.add_argument('--headless', action='store_true', help='replay and export without UI')
    args = parser.parse_args()
    if args.headless and not args.replay:
        parser.error('--headless requires --replay')
    model = Model()
    fd = None
    try:
        if args.replay:
            model.source = f'REPLAY (not live): {args.replay}'
            framer = Framer()
            with args.replay.open('rb') as stream:
                total = 0
                while chunk := stream.read(4096):
                    total += len(chunk)
                    if total > 16 * 1024 * 1024:
                        raise ValueError('replay exceeds 16 MiB limit')
                    ingest(model, framer, chunk)
            if framer.buffer or framer.discard:
                model.errors += 1
        else:
            model.source = f'SERIAL: {args.port}'
            fd = serial_open(args.port)
            model.connected = True
        if args.headless:
            model.export(args.export, time.monotonic())
            print(f'Exported {args.export}; invalid frames: {model.errors}')
        else:
            curses.wrapper(run_ui, model, fd, args.export)
    except (OSError, ValueError, curses.error) as error:
        parser.exit(1, f'Lease Watch: {error}\n')
    finally:
        if fd is not None:
            os.close(fd)


if __name__ == '__main__':
    main()
