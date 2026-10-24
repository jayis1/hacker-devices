#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise real C packets through replay and Linux serial/UI pseudo-terminals."""
import errno
import fcntl
import json
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import tempfile
import termios
import time

ROOT = Path(__file__).resolve().parents[1]


def drain(fd, seconds=0.4):
    end = time.monotonic() + seconds
    data = bytearray()
    while time.monotonic() < end:
        if select.select([fd], [], [], 0.05)[0]:
            try:
                data.extend(os.read(fd, 65536))
            except OSError as error:
                if error.errno == errno.EIO:
                    break
                raise
    return bytes(data)


def main():
    fixture = subprocess.check_output([str(ROOT / 'firmware/build/fixture')])
    with tempfile.TemporaryDirectory(prefix='lease-integration-') as directory:
        work = Path(directory)
        trace = work / 'synthetic.txt'
        trace.write_bytes(fixture)
        report = work / 'replay.json'
        subprocess.run(['python3', str(ROOT / 'app/lease.py'), '--replay', str(trace),
                        '--headless', '--export', str(report)], check=True)
        data = json.loads(report.read_text())
        assert data['author'] == 'jayis1' and data['parse_errors'] == 0
        assert data['status']['alarms'] == 2
        assert data['status']['heartbeats'] == 8
        assert not data['fresh'] and not data['connected']
        ui_master, ui_slave = pty.openpty()
        serial_master, serial_slave = pty.openpty()
        fcntl.ioctl(ui_slave, termios.TIOCSWINSZ, struct.pack('HHHH', 35, 120, 0, 0))
        export = work / 'live.json'
        process = subprocess.Popen(['python3', str(ROOT / 'app/lease.py'), '--port',
                                    os.ttyname(serial_slave), '--export', str(export)],
                                   stdin=ui_slave, stdout=ui_slave, stderr=ui_slave,
                                   env={**os.environ, 'TERM': 'xterm'})
        try:
            output = drain(ui_master)
            assert b'Lease Watch' in output
            os.write(serial_master, fixture)
            output += drain(ui_master)
            assert b'EXPIRED' in output and b'FRESH' in output
            os.write(ui_master, b'2')
            assert b'EVENTS' in drain(ui_master)
            os.write(ui_master, b'3')
            assert b'HEALTH' in drain(ui_master)
            os.write(ui_master, b'e')
            drain(ui_master)
            live = json.loads(export.read_text())
            assert live['connected'] and live['status']['alarms'] == 2
            assert (export.stat().st_mode & 0o777) == 0o600
            os.write(ui_master, b'e')
            assert b'failed' in drain(ui_master)  # curses retains the shared Export prefix
            assert json.loads(export.read_text()) == live
            os.close(serial_master)
            serial_master = None
            assert b'disconnected' in drain(ui_master)
            os.write(ui_master, b'q')
            process.wait(timeout=5)
            assert process.returncode == 0
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
            for fd in (ui_master, ui_slave, serial_master, serial_slave):
                if fd is not None:
                    os.close(fd)
    print('PASS: C encoder replay, serial PTY, three UI screens, private export, overwrite refusal, disconnect, quit')


if __name__ == '__main__':
    main()
