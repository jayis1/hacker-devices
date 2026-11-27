# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise actual Tk widgets and serial read loop with synthetic C evidence."""
import json
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
from console import Console
ROOT=Path(__file__).resolve().parents[1]
root=tk.Tk(); ui=Console(root)
def pump(seconds=.04):
    end=time.monotonic()+seconds
    while time.monotonic()<end:
        root.update(); time.sleep(.002)
master,slave=pty.openpty(); before=termios.tcgetattr(slave)
try:
    ui.path.set(os.ttyname(slave)); ui.connect(); assert ui.port
    first=ui.port; ui.connect(); assert ui.port is first
    data=subprocess.check_output([str(ROOT/'firmware/build/fixture'),'early'])
    for line in data.splitlines(keepends=True):
        os.write(master,line); pump(.024)
    assert ui.model.latest and ui.model.latest['faults']==1
    assert not ui.model.frozen and not ui.model.errors
    assert 'release before two later admissions' in ui.summary.get('1.0','end')
    assert len(ui.tabs.tabs())==3 and len(ui.model.history)==12
    ui.disconnect(); assert not ui.model.live()
    assert termios.tcgetattr(slave)==before
    with tempfile.TemporaryDirectory() as directory:
        recording=Path(directory)/'recording.rt1'; recording.write_bytes(data)
        ui.load(recording); pump()
        assert ui.model.origin=='replay' and ui.model.latest['faults']==1
        output=Path(directory)/'export.json'; ui.model.export(output)
        saved=json.loads(output.read_text()); assert saved['author']=='jayis1'
        assert saved['latest']['faults']==1 and not saved['connected']
    ui.path.set('/dev/null'); ui.connect(); assert not ui.port
    ui.path.set(os.ttyname(slave)); ui.connect(); assert ui.port
    os.close(master); master=None; pump()
    assert not ui.port and not ui.model.connected
    print('PASS: Tk ledger/history/integrity, C frames, PTY, reconnect, replay, export, hangup')
finally:
    ui.close(); root.destroy()
    if master is not None: os.close(master)
    os.close(slave)
