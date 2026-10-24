#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise the real curses screens and serial parser through independent PTYs."""
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

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='gray-integration-') as directory:
    directory = Path(directory)
    binary = directory/'fixture'
    subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-I'+str(root/'firmware'), str(root/'tests/fixture.c'),
                    str(root/'firmware/audit.c'), str(root/'firmware/wire.c'),
                    '-o', str(binary)], check=True)
    fixture = subprocess.check_output([str(binary)])
    ui, ui_slave = pty.openpty()
    serial, serial_slave = pty.openpty()
    fcntl.ioctl(ui_slave, termios.TIOCSWINSZ, struct.pack('HHHH', 35, 160, 0, 0))
    output = directory/'export.json'
    process = subprocess.Popen(['python3', str(root/'app/gray.py'), '--port',
                                os.ttyname(serial_slave), '--export', str(output)],
                               stdin=ui_slave, stdout=ui_slave, stderr=ui_slave,
                               env={**os.environ, 'TERM': 'xterm'})
    def read_for(seconds):
        end = time.monotonic()+seconds
        result = b''
        while time.monotonic() < end:
            if select.select([ui], [], [], 0.03)[0]:
                result += os.read(ui, 65536)
        assert process.poll() is None, result.decode(errors='replace')
        return result
    try:
        assert b'OVERVIEW' in read_for(0.3)
        os.write(serial, fixture[:20])
        read_for(0.1)
        os.write(serial, fixture[20:])
        rendered = read_for(0.4)
        assert b'position=40' in rendered, rendered
        os.write(ui, b'2')
        assert b'DISTRIBUTIONS' in read_for(0.2)
        os.write(ui, b'3')
        assert b'EVENTS' in read_for(0.2)
        os.write(ui, b'e')
        assert b'Export saved' in read_for(0.3)
        saved = output.read_bytes()
        data = json.loads(saved)
        assert data['rows']['S0']['values'][3] == 40
        assert data['rows']['S0']['values'][6] == 1
        assert data['rows']['S0']['values'][9] == 10
        assert data['author'] == 'jayis1'
        os.write(ui, b'e')
        assert b'refused' in read_for(0.2)
        assert output.read_bytes() == saved
        os.close(serial)
        serial = None
        assert b'disconnected' in read_for(0.3)
        os.write(ui, b'q')
        assert process.wait(timeout=3) == 0
        print('PASS: production C frames -> PTY serial -> live app; all screens, export, overwrite refusal, disconnect, quit')
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=3)
        for fd in (ui, ui_slave, serial, serial_slave):
            if fd is not None:
                os.close(fd)
