#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise real C frames, replay CLI, Linux serial PTY, and curses UI."""
import fcntl
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

ROOT=Path(__file__).resolve().parents[1]
frames=subprocess.check_output([str(ROOT/'firmware/build/fixture')])
with tempfile.TemporaryDirectory(prefix='handoff-integration-') as directory:
    work=Path(directory)
    capture=work/'synthetic.txt'
    capture.write_bytes(frames)
    replay=work/'replay.json'
    subprocess.run([sys.executable,str(ROOT/'app/audit.py'),'--replay',str(capture),
                    '--headless','--export',str(replay)],check=True)
    result=json.loads(replay.read_text())
    assert result['status']['transitions']==2
    assert result['status']['short_break']==1
    assert result['status']['feedback_overlap']==1
    assert result['status']['mismatch_a']==1
    assert result['replay'] and not result['fresh']
    # Loss of the real encoder's initial B record is recoverable through P.
    capture.write_bytes(frames.split(b'\n',1)[1])
    late=work/'late.json'
    subprocess.run([sys.executable,str(ROOT/'app/audit.py'),'--replay',str(capture),
                    '--headless','--export',str(late)],check=True)
    recovered=json.loads(late.read_text())
    assert recovered['policy_known'] and recovered['boot'] is None
    assert recovered['status']['transitions']==2
    serial_master,serial_slave=pty.openpty()
    ui_master,ui_slave=pty.openpty()
    fcntl.ioctl(ui_slave,termios.TIOCSWINSZ,struct.pack('HHHH',32,120,0,0))
    export=work/'live.json'
    env=dict(os.environ,TERM='xterm')
    process=subprocess.Popen([sys.executable,str(ROOT/'app/audit.py'),'--port',
                              os.ttyname(serial_slave),'--export',str(export)],
                             stdin=ui_slave,stdout=ui_slave,stderr=ui_slave,env=env)
    output=bytearray()
    def drain(seconds=0.2):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            if select.select([ui_master],[],[],0.05)[0]:
                output.extend(os.read(ui_master,65536))
    try:
        drain()
        assert b'Handoff Audit' in output
        os.write(serial_master,frames)
        drain()
        os.write(ui_master,b'e')
        drain()
        live=json.loads(export.read_text())
        assert live['status']['transitions']==2 and live['fresh'] and live['connected']
        assert os.stat(export).st_mode&0o777==0o600
        os.write(ui_master,b'2')
        drain()
        assert b'EVENT HISTORY' in output
        os.write(ui_master,b'3')
        drain()
        assert b'HEALTH' in output
        os.write(ui_master,b'e')
        drain()
        assert b'refused' in output
        assert json.loads(export.read_text())==live
        os.close(serial_master)
        serial_master=None
        drain()
        assert b'disconnected' in output
        os.write(ui_master,b'q')
        assert process.wait(timeout=5)==0
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        for fd in [serial_master,serial_slave,ui_master,ui_slave]:
            if fd is not None: os.close(fd)
print('PASS: C encoder -> replay export; live PTY -> screens/export/refusal/disconnect/exit')
