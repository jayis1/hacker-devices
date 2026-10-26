# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Real Tk event loop and PTY integration, synthetic C-encoder data."""
from test_app import app, fixture
import json
import os
from pathlib import Path
import pty
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch

class GUI(unittest.TestCase):
    def test_session(self):
        root=tk.Tk(); gui=app.Console(root)
        master,slave=pty.openpty()
        def pump(duration):
            end=time.monotonic()+duration
            while time.monotonic()<end:
                root.update(); time.sleep(0.005)
        try:
            gui.port.set(os.ttyname(slave)); gui.connect()
            # Stream at device cadence: accelerated replay is not live time.
            lines=fixture().splitlines(keepends=True)
            for line in lines:
                os.write(master,line)
                if line.startswith(b'BB1,S,'): pump(0.1)
            pump(0.05)
            self.assertEqual(gui.model.status['excess'],2)
            self.assertFalse(gui.model.frozen)
            self.assertTrue(gui.model.fresh())
            for i in range(3): gui.book.select(i); pump(0.02)
            self.assertIn('Allowed: 4',gui.views[0].get('1.0','end'))
            self.assertIn('4500',gui.views[1].get('1.0','end'))
            with tempfile.TemporaryDirectory() as directory:
                report=Path(directory)/'report.json'
                with patch.object(app.filedialog,'asksaveasfilename',return_value=str(report)):
                    gui.export()
                    self.assertEqual(json.loads(report.read_text())['source'],'live')
                    with patch.object(app.messagebox,'showerror') as error:
                        gui.export(); self.assertTrue(error.called)
                os.close(master); master=None; pump(0.2)
                self.assertFalse(gui.model.connected)
                self.assertIsNone(gui.serial)
                self.assertEqual(gui.model.source,'live')
                recording=Path(directory)/'fixture.txt'; recording.write_bytes(fixture())
                gui.load(recording); pump(0.02)
                self.assertEqual(gui.model.source,'replay')
                self.assertFalse(gui.model.fresh())
                self.assertEqual(len(gui.model.history),46)
                # Reconnection resets numeric view before receiving data.
                master,other=pty.openpty()
                gui.port.set(os.ttyname(other)); gui.connect(); pump(0.02)
                self.assertIsNone(gui.model.status)
                self.assertIn('No accepted status',gui.views[0].get('1.0','end'))
                os.close(other)
        finally:
            gui.close(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
