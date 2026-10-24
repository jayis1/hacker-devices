#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Gray Audit: local receive-only Linux serial dashboard; no third-party packages."""
import argparse
import binascii
from collections import deque
import curses
import json
import os
from pathlib import Path
import re
import select
import termios
import time

COUNTS = {'B': 2, 'S': 17, 'D': 14, 'H': 7, 'E': 6}
NAMES = ['ms', 'axis', 'state', 'position', 'forward', 'reverse', 'illegal',
         'reversals', 'edges', 'cycles_forward', 'cycles_reverse', 'phase_a',
         'phase_b', 'edge_age_ms', 'min_interval_ms', 'max_interval_ms', 'saturated']
MAX_LINE = 383


def parse(raw):
    if not raw.endswith(b'\n') or len(raw) > MAX_LINE:
        raise ValueError('framing')
    try:
        body, checksum = raw[:-1].rsplit(b'*', 1)
        if not re.fullmatch(rb'[0-9A-F]{4}', checksum):
            raise ValueError('checksum syntax')
        if binascii.crc_hqx(body, 0xffff) != int(checksum, 16):
            raise ValueError('checksum mismatch')
        words = body.decode('ascii').split(',')
        if len(words) < 2 or words[0] != 'GA1' or words[1] not in COUNTS:
            raise ValueError('version/type')
        kind = words[1]
        if len(words) != COUNTS[kind] + 2:
            raise ValueError('field count')
        values = []
        for i, word in enumerate(words[2:]):
            signed = kind == 'S' and i == 3
            if not re.fullmatch(r'-?(0|[1-9][0-9]{0,9})' if signed else
                                r'(0|[1-9][0-9]{0,9})', word):
                raise ValueError('number syntax')
            value = int(word)
            if not ((-2147483648 <= value <= 2147483647) if signed else
                    (0 <= value <= 4294967295)):
                raise ValueError('number range')
            values.append(value)
        if kind in ('S', 'D') and values[1] not in (0, 1):
            raise ValueError('axis')
        if kind == 'S' and (values[2] > 3 or values[16] > 1):
            raise ValueError('state')
        if kind == 'E' and (values[2] > 1 or values[3] not in (1, 2, 3, 4)
                            or values[4] > 3 or values[5] > 3):
            raise ValueError('event')
        return kind, values
    except (UnicodeError, IndexError) as exc:
        raise ValueError('invalid frame') from exc


class Stream:
    def __init__(self):
        self.buffer = bytearray()
        self.discard = False
        self.errors = 0

    def feed(self, data):
        results = []
        for byte in data:
            if self.discard:
                if byte == 10:
                    self.discard = False
                continue
            self.buffer.append(byte)
            if len(self.buffer) > MAX_LINE:
                self.buffer.clear()
                self.discard = byte != 10
                self.errors += 1
            elif byte == 10:
                try:
                    results.append(parse(bytes(self.buffer)))
                except ValueError:
                    self.errors += 1
                self.buffer.clear()
        return results


class Model:
    def __init__(self):
        self.rows = {}
        self.events = deque(maxlen=128)
        self.last_ms = None
        self.resets = 0
        self.boot_flags = None
        self.records = 0

    def accept(self, kind, values, received):
        # Queued events can predate a status row; only periodic rows order clocks.
        periodic = kind in ('S', 'D', 'H')
        rollback = (periodic and self.last_ms is not None and
                    (values[0] - self.last_ms) % 2**32 > 2**31)
        if kind == 'B' or rollback:
            if self.records:
                self.resets += 1
            self.rows.clear()
            self.events.clear()
            self.last_ms = None
        if kind == 'B':
            self.boot_flags = values[1]
        if periodic:
            self.last_ms = values[0]
        if kind == 'E':
            self.events.append({'values': values, 'received': received})
        else:
            key = f'{kind}{values[1]}' if kind in ('S', 'D') else kind
            self.rows[key] = {'values': values, 'received': received}
        self.records += 1

    def snapshot(self, now, connection, errors):
        def row(r):
            return {'values': r['values'], 'age_seconds': max(0, now-r['received'])}
        return {'author': 'jayis1', 'protocol': 'GA1', 'connection': connection,
                'clock_resets_observed_or_inferred': self.resets,
                'parser_errors': errors, 'records': self.records,
                'rows': {k: row(v) for k, v in self.rows.items()},
                'events': [row(e) for e in self.events],
                'note': 'Unauthenticated observations, not forensic proof; no wall-clock attestation.'}


def export(model, path, now, connection, errors):
    # O_EXCL refuses overwrite and symlink following; permissions ignore lax umasks.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as out:
        json.dump(model.snapshot(now, connection, errors), out, indent=2)
        out.write('\n')


def open_serial(path):
    fd = os.open(path, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
        attrs[3] = 0
        attrs[4] = attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
    except BaseException:
        os.close(fd)
        raise
    return fd


def screen_lines(model, tab, now):
    if tab == '1':
        lines = ['OVERVIEW: raw observed transitions; NOT validated shaft position']
        for index in range(2):
            r = model.rows.get(f'S{index}')
            if not r:
                lines.append(f'Axis {index}: waiting for status')
                continue
            age = max(0, now - r['received'])
            lines.append(f'Axis {index} | age {age:.1f}s | ' + ('STALE' if age > 2 else 'recent'))
            v = dict(zip(NAMES, r['values']))
            for keys in (('state', 'position', 'forward', 'reverse'),
                         ('illegal', 'reversals', 'edges'),
                         ('cycles_forward', 'cycles_reverse'),
                         ('phase_a', 'phase_b', 'edge_age_ms'),
                         ('min_interval_ms', 'max_interval_ms', 'saturated')):
                lines.append('  ' + '  '.join(f'{k}={v[k]}' for k in keys))
        h = model.rows.get('H')
        if h:
            lines.append(f'Health age {now-h["received"]:.1f}s: ms,samples,gaps,missed,dropped,tx_failed,sequence')
            lines.append('  ' + ','.join(map(str, h['values'])))
        return lines
    if tab == '2':
        lines = ['DISTRIBUTIONS: dwell ms by state; edge intervals by power-of-two bin']
        for index in range(2):
            r = model.rows.get(f'D{index}')
            if not r:
                lines.append(f'Axis {index}: waiting for distribution')
                continue
            lines.append(f'Axis {index} age {now-r["received"]:.1f}s')
            lines.append('Dwell 00,01,10,11: ' + str(r['values'][2:6]))
            lines.append('Bins 0-1,2-3,4-7,8-15,16-31,32-63,64-127,128+:')
            lines.append(str(r['values'][6:]))
        return lines
    return ['EVENTS: ms,sequence,axis,kind,before,after (1 illegal; 2 reversal; 3 gap; 4 saturation)'] + [
        str(e['values']) for e in list(model.events)[-15:]]


def run_ui(stdscr, fd, output):
    stream, model = Stream(), Model()
    tab, message, connection = '1', '', 'connected; awaiting data'
    stdscr.nodelay(True)
    while True:
        if fd is not None and select.select([fd], [], [], 0)[0]:
            try:
                data = os.read(fd, 4096)
                if not data:
                    raise OSError('serial EOF')
                for kind, values in stream.feed(data):
                    model.accept(kind, values, time.monotonic())
                connection = 'connected'
            except OSError:
                os.close(fd)
                fd = None
                connection = 'disconnected; restart app to reconnect'
        now = time.monotonic()
        lines = ['Gray Audit | author jayis1 | 1 overview 2 distributions 3 events e export q quit',
                 f'{connection} | rejected {stream.errors} | resets {model.resets}',
                 message] + screen_lines(model, tab, now)
        stdscr.erase()
        height, width = stdscr.getmaxyx()
        for y, line in enumerate(lines[:max(0, height-1)]):
            try:
                stdscr.addnstr(y, 0, line, max(0, width-1))
            except curses.error:
                pass
        stdscr.refresh()
        key = stdscr.getch()
        if key == ord('q'):
            return fd
        if key in (ord('1'), ord('2'), ord('3')):
            tab = chr(key)
        if key == ord('e'):
            try:
                export(model, output, now, connection, stream.errors)
                message = 'Export saved (private, no overwrite): ' + str(output)
            except OSError as exc:
                message = 'Export refused: ' + str(exc)
        time.sleep(0.03)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='Linux TTY, e.g. /dev/ttyACM0')
    parser.add_argument('--export', type=Path, required=True, help='New local JSON file; never overwritten')
    args = parser.parse_args()
    fd = open_serial(args.port)
    try:
        fd = curses.wrapper(run_ui, fd, args.export)
    finally:
        if fd is not None:
            try:
                os.close(fd)
            except OSError:
                pass


if __name__ == '__main__':
    main()
