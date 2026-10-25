# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise real Tk screens and termios receive path using a synthetic PTY."""
import json
import os
from pathlib import Path
import pty
import tempfile
import time
import tkinter as tk
from test_app import audit, fixture

def pump(root, seconds=0.15):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        root.update()
        time.sleep(0.005)

with tempfile.TemporaryDirectory() as tmp:
    root=tk.Tk()
    console=audit.Console(root)
    master,slave=pty.openpty()
    console.path.set(os.ttyname(slave))
    console.connect()
    assert console.model.connected
    os.write(master,fixture())
    pump(root)
    assert console.model.latest['downgrades']==1
    assert console.model.errors==0
    assert len(console.tabs.tabs())==3
    assert 'Highest observed: 2' in console.views['Epochs'].get('1.0','end')
    assert 'Generation rollback' in console.views['Epochs'].get('1.0','end')
    events=console.views['Events'].get('1.0','end')
    assert len(events.strip().splitlines())==len(console.model.events)+1
    console.tabs.select(1); pump(root)
    console.tabs.select(2); pump(root)
    os.close(master); os.close(slave)
    pump(root)
    assert not console.model.connected
    assert console.model.source=='live'
    report=Path(tmp)/'report.json'
    console.model.export(report)
    snap=json.loads(report.read_text())
    assert snap['author']=='jayis1' and snap['source']=='live'
    assert not snap['connected']
    replay=Path(tmp)/'fixture.ew1'; replay.write_bytes(fixture())
    console.load_replay(replay); pump(root)
    assert console.model.source=='replay'
    assert console.model.latest['downgrades']==1
    console.path.set('/nonexistent-epoch-device')
    console.connect(); pump(root)
    assert console.model.latest is None
    assert 'No accepted status' in console.views['Epochs'].get('1.0','end')
    assert not console.model.connected
    fifo=Path(tmp)/'fifo'; os.mkfifo(fifo)
    console.load_replay(fifo); pump(root)
    assert 'regular file' in console.model.warning
    console.close()
print('Tk screens, PTY live input/hangup, private export, replay and cleared reconnect PASS')
