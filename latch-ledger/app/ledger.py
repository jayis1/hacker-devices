#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Local-only Linux terminal companion; no network services or target commands."""
import argparse
import collections
import curses
import json
import os
import pathlib
import select
import stat
import termios
import time

MAX_LINE = 255
COUNTS = {'B': 3, 'E': 6, 'S': 9, 'P': 5, 'H': 5}


def crc(data):
    value = 0xFFFF
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            value = ((value << 1) ^ (0x1021 if value & 0x8000 else 0)) & 0xFFFF
    return value


def parse(line):
    body, check = line.decode('ascii').split('*')
    if len(check) != 4 or int(check, 16) != crc(body.encode('ascii')):
        raise ValueError('checksum')
    fields = body.split(',')
    if len(fields) < 2 or fields[0] != 'LL1' or fields[1] not in COUNTS:
        raise ValueError('protocol')
    kind = fields[1]
    if len(fields) != 2 + COUNTS[kind]:
        raise ValueError('field count')
    values = []
    for value in fields[2:]:
        if not value or not value.isdecimal() or len(value) > 10:
            raise ValueError('number')
        number = int(value)
        if number > 0xFFFFFFFF:
            raise ValueError('overflow')
        values.append(number)
    if kind == 'B' and values[1:] != [5, 1]:
        raise ValueError('unsupported firmware or hardware')
    if kind == 'S' and (values[1] > 3 or values[2] > 1 or values[3] > 1):
        raise ValueError('channel status')
    if kind == 'P' and (values[1] > 1 or values[2] > 1):
        raise ValueError('pair status')
    if kind == 'E':
        _, _, event, channel, level, _ = values
        if event not in (1, 2, 3) or channel > (1 if event == 3 else 3) or level > 1:
            raise ValueError('event')
    return kind, values


class Decoder:
    def __init__(self):
        self.pending = bytearray()
        self.discarding = False
        self.errors = 0

    def feed(self, data):
        records = []
        for byte in data:
            if byte == 10:
                if not self.discarding:
                    try:
                        records.append(parse(bytes(self.pending)))
                    except (ValueError, UnicodeError):
                        self.errors += 1
                self.pending.clear()
                self.discarding = False
            elif not self.discarding:
                if len(self.pending) >= MAX_LINE:
                    self.pending.clear()
                    self.discarding = True
                    self.errors += 1
                else:
                    self.pending.append(byte)
        return records


class Model:
    def __init__(self):
        self.channels = {}
        self.pairs = {}
        self.events = collections.deque(maxlen=500)
        self.health = None
        self.sequence = None
        self.gaps = 0
        self.sessions = 0
        self.last_seen = None
        self.reset_flags = None

        self.channel_seen = {}
        self.pair_seen = {}
        self.inferred_restarts = 0
        self.connection = 'not connected / replay'

    def accept(self, record):
        kind, values = record
        if kind == 'H' and self.health is not None:
            previous = self.health[0]
            if values[0] < previous and previous - values[0] < 0x80000000:
                self.channels.clear()
                self.pairs.clear()
                self.channel_seen.clear()
                self.pair_seen.clear()
                self.events.clear()
                self.sequence = None
                self.reset_flags = None
                self.inferred_restarts += 1
        self.last_seen = time.monotonic()
        if kind == 'B':
            self.channels.clear()
            self.pairs.clear()
            self.channel_seen.clear()
            self.pair_seen.clear()
            self.events.clear()
            self.health = None
            self.sequence = None
            self.gaps = 0
            self.sessions += 1
            self.reset_flags = values[0]
        elif kind == 'E':
            sequence = values[0]
            if self.sequence is not None:
                expected = (self.sequence + 1) & 0xFFFFFFFF
                if sequence != expected:
                    self.gaps += 1
            self.sequence = sequence
            self.events.append(values)
        elif kind == 'S':
            self.channels[values[1]] = values
            self.channel_seen[values[1]] = self.last_seen
        elif kind == 'P':
            self.pairs[values[1]] = values
            self.pair_seen[values[1]] = self.last_seen
        elif kind == 'H':
            self.health = values

    def snapshot(self, errors):
        now = time.monotonic()
        return {'author': 'jayis1', 'format': 'latch-ledger-view-1',
                'connection': self.connection, 'inferred_restarts': self.inferred_restarts,
                'channel_age_seconds': {k: now-v for k, v in self.channel_seen.items()},
                'pair_age_seconds': {k: now-v for k, v in self.pair_seen.items()},
                'sessions_observed': self.sessions, 'reset_flags': self.reset_flags,
                'channels': self.channels, 'pairs': self.pairs,
                'health': self.health, 'sequence_gap_incidents': self.gaps,
                'decoder_errors': errors, 'recent_events': list(self.events),
                'note': 'Unsigned, bounded local view; not a complete forensic log.'}


class Serial:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            if not stat.S_ISCHR(os.fstat(self.fd).st_mode) or not os.isatty(self.fd):
                raise ValueError('Expected a local serial TTY')
            self.previous = termios.tcgetattr(self.fd)
            attributes = termios.tcgetattr(self.fd)
            attributes[0] = 0
            attributes[1] = 0
            attributes[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
            attributes[3] = 0
            attributes[4] = termios.B115200
            attributes[5] = termios.B115200
            attributes[6][termios.VMIN] = 0
            attributes[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, attributes)
        except Exception:
            os.close(self.fd)
            raise

    def read(self):
        if not select.select([self.fd], [], [], 0)[0]:
            return None
        try:
            return os.read(self.fd, 4096)
        except BlockingIOError:
            return None

    def close(self):
        try:
            termios.tcsetattr(self.fd, termios.TCSANOW, self.previous)
        except termios.error:
            pass  # Unplugged hardware cannot restore termios.
        os.close(self.fd)


def export_view(path, model, decoder):
    # Exclusive creation and mode 0600 avoid clobbering files or exposing logs.
    fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w', encoding='utf-8') as output:
        json.dump(model.snapshot(decoder.errors), output, indent=2)
        output.write('\n')


def display(screen, model, decoder, status, tab, message):
    screen.erase()
    height, width = screen.getmaxyx()
    def line(row, text):
        if row < height - 1 and width > 1:
            screen.addnstr(row, 0, text, width - 1)
    line(0, 'Latch Ledger | Author: jayis1 | 1:channels 2:events 3:help e:export q:quit')
    age = 'no valid records' if model.last_seen is None else f'{time.monotonic() - model.last_seen:.1f}s since record'
    stale = model.last_seen is None or time.monotonic() - model.last_seen > 2
    line(1, f'{status} | {"STALE/UNKNOWN" if stale else "receiving"} | {age}')
    line(2, f'CRC/schema errors={decoder.errors} gap incidents={model.gaps} boots={model.sessions}')
    if tab == '1':
        line(4, 'CH  RAW  STABLE   EDGES   CHANGES   SHORT<5ms   LONGEST OPEN ms')
        for channel in range(4):
            item = model.channels.get(channel)
            if item is None:
                line(5 + channel, f'{channel}   unknown (waiting for status)')
            else:
                _, ch, raw, stable, edges, changes, short, longest, _ = item
                old = time.monotonic() - model.channel_seen[channel] > 2
                line(5 + channel, f'{ch}   {raw}    {stable}        {edges}        {changes}         {short}           {longest} {"STALE" if old else "last observed"}')
        for pair in range(2):
            old = time.monotonic() - model.pair_seen.get(pair, 0) > 2
            line(10 + pair, f'Pair {2 * pair}/{2 * pair + 1}: {model.pairs.get(pair, "unknown")} {"STALE" if old else "last observed"}')
        line(13, f'Health [ms,seq,dropped,missed,tx_fail]: {model.health}')
        line(15, '1 = OPEN; 0 = CLOSED. Initial levels are not measured transitions.')
    elif tab == '2':
        line(4, 'Recent events: [sequence, ms, kind(1 raw/2 stable/3 pair), channel, level, duration]')
        for row, item in enumerate(list(model.events)[-max(0, height - 8):], 5):
            line(row, str(item))
    else:
        help_lines = [
            'Authorized bench testing of disconnected, voltage-free contacts ONLY.',
            'Never attach to live alarms, locks, mains, safety circuits, or powered GPIO.',
            'Use short leads. No isolation, surge protection, or compliance rating.',
            'Pair 0/1 and pair 2/3 compare debounced states, not physical causation.',
            'Sampling target is 1 ms; sub-ms pulses are not guaranteed detectable.',
            'Missed samples, sequence gaps and queue drops invalidate completeness.',
            'CRC detects damage; it does NOT authenticate the device or evidence.',
            'No commands, cloud services, automatic reconnect, or silent recording.',
            'Export is a bounded snapshot, creates a new private file, never overwrites.',
            'Boot starts a new volatile session; reconnect requires restarting app.',
        ]
        for row, text in enumerate(help_lines, 4):
            line(row, text)
    line(height - 2, message)
    screen.refresh()


def interactive(screen, args):
    screen.nodelay(True)
    decoder, model = Decoder(), Model()
    connection = None
    status, message, tab = 'DISCONNECTED', '', '1'
    try:
        try:
            connection = Serial(args.port)
            status = f'OPEN {args.port}'
            model.connection = status
        except (OSError, ValueError, termios.error) as error:
            message = f'Open failed: {error}'
        while True:
            if connection:
                try:
                    data = connection.read()
                    if data == b'':
                        raise OSError('serial disconnected')
                    if data:
                        for record in decoder.feed(data):
                            model.accept(record)
                except (OSError, termios.error) as error:
                    connection.close()
                    connection = None
                    status = 'DISCONNECTED'
                    model.connection = status
                    message = str(error)
            display(screen, model, decoder, status, tab, message)
            key = screen.getch()
            if key == ord('q'):
                break
            if key in (ord('1'), ord('2'), ord('3')):
                tab = chr(key)
            if key == ord('e'):
                try:
                    export_view(args.export, model, decoder)
                    message = f'Exported {args.export}'
                except OSError as error:
                    message = f'Export failed: {error}'
            time.sleep(0.02)
    finally:
        if connection:
            connection.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--port', help='Linux serial TTY, usually /dev/ttyACM0')
    source.add_argument('--replay', type=pathlib.Path, help='Analyze saved raw LL1 stream without hardware')
    parser.add_argument('--export', default='latch-ledger-view.json')
    args = parser.parse_args()
    if args.replay:
        decoder, model = Decoder(), Model()
        with args.replay.open('rb') as source_file:
            while data := source_file.read(4096):
                for record in decoder.feed(data):
                    model.accept(record)
        if decoder.pending or decoder.discarding:
            decoder.errors += 1
        print(json.dumps(model.snapshot(decoder.errors), indent=2))
        return 1 if decoder.errors else 0
    curses.wrapper(interactive, args)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
