# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise real Tk widgets, Linux PTY transport, and firmware encoder fixtures."""
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import tkinter as tk
import unittest
from test_app import app, ROOT

class GuiTests(unittest.TestCase):
    def test_live_replay_export_hangup(self):
        root=tk.Tk()
        ui=app.Console(root)
        master,slave=pty.openpty()
        ui.path.set(os.ttyname(slave))
        ui.connect()
        self.assertIsNotNone(ui.serial)
        data=subprocess.check_output([str(ROOT/'firmware/build/fixture')])
        # Keep each write below the PTY capacity and let the event loop drain.
        for line in data.splitlines(keepends=True):
            os.write(master,line)
            deadline=time.monotonic()+0.03
            while time.monotonic()<deadline:
                root.update()
                time.sleep(0.002)
        self.assertEqual(len(ui.model.rows),121)
        self.assertIn('3000',ui.views['Budget'].get('1.0','end'))
        for i in range(3):
            ui.tabs.select(i)
            root.update()
        time.sleep(1.1)
        root.update()
        self.assertTrue(ui.model.evidence(True)['stale'])
        os.close(master)
        for _ in range(10):
            root.update()
            time.sleep(0.03)
        self.assertIsNone(ui.serial)
        self.assertEqual(len(ui.model.rows),121)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'capture.json'
            ui.model.export(p,False)
            self.assertIn('live:',p.read_text())
            self.assertIn('"connected": false',p.read_text())
            replay=Path(d)/'synthetic.txt'
            replay.write_bytes(data)
            ui.load(replay)
            self.assertIn('replay:',ui.model.provenance)
            self.assertEqual(len(ui.model.rows),121)
        os.close(slave)
        ui.close()

if __name__=='__main__': unittest.main()
