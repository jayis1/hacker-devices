#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Exercise actual curses screens and serial disconnect using Linux PTYs."""
import fcntl
import importlib.util
import json
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import sys
import tempfile
import termios
import time

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('ledger', root / 'app/ledger.py')
assert spec is not None and spec.loader is not None
ledger = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ledger)


def drain(fd, seconds):
    result = bytearray()
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        if select.select([fd], [], [], 0.02)[0]:
            try:
                result.extend(os.read(fd, 65536))
            except OSError:
                break
    return bytes(result)


with tempfile.TemporaryDirectory(prefix='latch-ui-') as directory:
    directory = Path(directory)
    fixture_exe = directory / 'fixture'
    subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(root / 'firmware'), str(root / 'tests/fixture.c'),
                    str(root / 'firmware/monitor.c'), str(root / 'firmware/wire.c'),
                    '-o', str(fixture_exe)], check=True)
    fixture = subprocess.check_output([str(fixture_exe)])
    decoder = ledger.Decoder()
    assert len(decoder.feed(fixture)) == 14 and decoder.errors == 0
    raw = directory / 'synthetic.ll1'
    raw.write_bytes(fixture)
    replay = json.loads(subprocess.check_output([sys.executable, str(root / 'app/ledger.py'),
                                                '--replay', str(raw)]))
    assert replay['channels']['0'][3] == 1
    assert replay['pairs']['0'][4] == 10
    serial_master, serial_slave = pty.openpty()
    terminal_master, terminal_slave = pty.openpty()
    fcntl.ioctl(terminal_slave, termios.TIOCSWINSZ, struct.pack('HHHH', 30, 160, 0, 0))
    output = directory / 'view.json'
    child = subprocess.Popen([sys.executable, str(root / 'app/ledger.py'), '--port',
                              os.ttyname(serial_slave), '--export', str(output)],
                             stdin=terminal_slave, stdout=terminal_slave, stderr=terminal_slave,
                             env={**os.environ, 'TERM': 'xterm'})
    try:
        screen = drain(terminal_master, 0.3)
        os.write(serial_master, fixture)
        screen += drain(terminal_master, 0.3)
        assert b'Latch Ledger' in screen and b'OPEN /dev/pts/' in screen
        os.write(terminal_master, b'2')
        events = drain(terminal_master, 0.2)
        assert b'Recent events' in events
        os.write(terminal_master, b'3')
        help_screen = drain(terminal_master, 0.2)
        assert b'Authorized bench testing' in help_screen
        os.write(terminal_master, b'e')
        drain(terminal_master, 0.2)
        saved = json.loads(output.read_text())
        assert saved['channels']['0'][3] == 1 and saved['decoder_errors'] == 0
        assert output.stat().st_mode & 0o777 == 0o600
        os.write(terminal_master, b'e')
        assert b'File exists' in drain(terminal_master, 0.2)
        assert json.loads(output.read_text()) == saved
        os.close(serial_master)
        serial_master = None
        assert b'DISCONNECTED' in drain(terminal_master, 0.3)
        os.write(terminal_master, b'q')
        child.wait(timeout=3)
        assert child.returncode == 0
    finally:
        if child.poll() is None:
            child.kill()
            child.wait()
        for fd in [serial_master, serial_slave, terminal_master, terminal_slave]:
            if fd is not None:
                os.close(fd)
    print('PASS: C-to-app replay, live PTY, three screens, private export, overwrite refusal, disconnect, clean exit')
