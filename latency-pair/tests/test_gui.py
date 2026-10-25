# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Real Tk event-loop tests; PTY transport is synthetic, not USB hardware."""
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import tkinter as tk
import unittest
from test_app import app, ROOT

class Gui(unittest.TestCase):
    def test_live_replay_export_hangup(self):
        root=tk.Tk()
        ui=app.Console(root)
        master,slave=pty.openpty()
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        def pump(duration=0.15):
            until=time.monotonic()+duration
            while time.monotonic()<until:
                root.update(); time.sleep(0.005)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            os.write(master,data); pump()
            self.assertEqual(len(ui.model.rows),32)
            self.assertEqual(ui.model.errors,0)
            self.assertIn('Completed pairs: 32 / 32',ui.views['Pairs'].get('1.0','end'))
            for name in ui.views:
                ui.tabs.select(ui.views[name]); pump(0.03)
            self.assertEqual(len(ui.views['History'].get('1.0','end').strip().splitlines()),33)
            pump(1.05)
            self.assertTrue(ui.model.evidence(True)['stale'])
            with tempfile.TemporaryDirectory() as d:
                replay=Path(d)/'synthetic.lp1'; replay.write_bytes(data)
                ui.model.export(Path(d)/'live.json',True)
                os.close(master); master=-1; pump()
                self.assertIsNone(ui.serial)
                self.assertTrue(ui.model.provenance.startswith('live:'))
                ui.load(replay)
                self.assertTrue(ui.model.provenance.startswith('replay:'))
                self.assertFalse(ui.model.frozen)
                ui.disconnect()
                ui.model.export(Path(d)/'replay.json',False)
                with self.assertRaises(FileExistsError): ui.model.export(Path(d)/'replay.json',False)
                replay.write_bytes(data+b'LP1,S,')
                ui.load(replay); self.assertTrue(ui.model.frozen)
                # Successful reconnect must clear old numeric results before data.
                other_master,other_slave=pty.openpty()
                try:
                    ui.path.set(os.ttyname(other_slave)); ui.connect(); pump()
                    self.assertFalse(ui.model.rows)
                    self.assertIn('No compatible',ui.views['Pairs'].get('1.0','end'))
                finally:
                    ui.disconnect(); os.close(other_master); os.close(other_slave)
        finally:
            ui.close()
            if master>=0: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
