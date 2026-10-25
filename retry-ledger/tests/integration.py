# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise actual Tk screens and Linux serial PTY; synthetic C-encoder data."""
import json
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import tkinter as tk
from test_app import audit

root=tk.Tk()
ui=audit.Console(root)
fixture=subprocess.check_output([str(Path(__file__).resolve().parents[1]/'firmware/build/fixture')])
def pump(seconds=.2):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        root.update()
        time.sleep(.005)

with tempfile.TemporaryDirectory() as directory:
    directory=Path(directory)
    replay=directory/'scenario.rl1'; replay.write_bytes(fixture)
    ui.load_replay(replay); pump()
    assert ui.model.latest['failures']==3
    assert 'Strikes: 0 / 3' in ui.views['Budget'].get('1.0','end')
    for index in range(3):
        ui.tabs.select(index); pump(.05)
    assert len(ui.model.events)>5
    assert 'replay' in ui.views['Integrity'].get('1.0','end')
    ui.model.export(directory/'replay.json')
    assert json.loads((directory/'replay.json').read_text())['source']=='replay'
    master,slave=pty.openpty()
    ui.path.set(os.ttyname(slave)); ui.connect(); pump()
    assert ui.model.latest is None and ui.model.connected
    # Small batches avoid blocking the PTY's finite kernel queue.
    for start in range(0,len(fixture),512):
        os.write(master,fixture[start:start+512]); pump(.06)
    pump()
    assert ui.model.latest is not None
    assert ui.model.latest['failures']==3 and not ui.model.blocked
    assert 'live' in ui.banner.get()
    pump(1.1)
    assert 'STALE' in ui.banner.get()
    os.close(master); pump()
    assert not ui.model.connected and ui.serial is None
    ui.model.export(directory/'unplug.json')
    report=json.loads((directory/'unplug.json').read_text())
    assert report['source']=='live' and report['stale'] and not report['connected']
    os.close(slave)
    ui.path.set('/does/not/exist'); ui.connect(); pump()
    assert not ui.model.connected and ui.model.latest is None
    fifo=directory/'fifo'; os.mkfifo(fifo)
    ui.load_replay(fifo); pump()
    assert 'regular file' in ui.model.warning
    huge=directory/'huge'
    with huge.open('wb') as stream: stream.truncate(2*1024*1024+1)
    try: audit.read_replay(huge)
    except ValueError: pass
    else: raise AssertionError('accepted oversized replay')
ui.close()
print('PASS: Tk Budget/Events/Integrity, replay, live PTY, export, stale, unplug, absent port, FIFO and size rejection')
