# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import os
from pathlib import Path
import pty
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import app, fixture
class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=app.Window(self.root)
    def tearDown(self):
        self.ui.close()
    def pump(self, seconds=.15):
        until=time.monotonic()+seconds
        while time.monotonic()<until:
            self.root.update(); time.sleep(.005)
    def test_live_export_disconnect_and_replay(self):
        master,slave=pty.openpty()
        try:
            self.ui.path.set(os.ttyname(slave)); self.ui.authorized.set(True)
            self.ui.connect()
            os.write(master,fixture()); self.pump()
            self.assertEqual(len(self.ui.model.rows),11)
            self.assertIn('switches: 1',self.ui.screens['Fairness'].get('1.0','end'))
            self.assertIn('LIVE',self.ui.status.get())
            for index in range(3): self.ui.tabs.select(index); self.root.update()
            with tempfile.TemporaryDirectory() as d:
                out=Path(d)/'out.json'
                with patch.object(app.filedialog,'asksaveasfilename',return_value=str(out)):
                    self.ui.save()
                self.assertTrue(out.exists())
                source=self.ui.model.source
                os.close(master); master=None; self.pump()
                self.assertFalse(self.ui.model.connected)
                self.assertEqual(self.ui.model.source,source)
                replay=Path(d)/'trace'; replay.write_bytes(fixture())
                with patch.object(app.filedialog,'askopenfilename',return_value=str(replay)):
                    self.ui.replay()
                self.assertEqual(len(self.ui.model.rows),11)
                self.assertTrue(self.ui.model.source.startswith('replay:'))
        finally:
            if master is not None: os.close(master)
            os.close(slave)
    def test_backlog_and_unavailable_port(self):
        self.ui.authorized.set(True); self.ui.path.set('/does/not/exist')
        with patch.object(app.messagebox,'showerror') as errors:
            self.ui.connect(); self.assertTrue(errors.called)
        master,slave=pty.openpty()
        try:
            self.ui.path.set(os.ttyname(slave)); self.ui.connect()
            os.write(master,fixture()); self.pump()
            self.ui.last_poll-=2
            self.pump()
            self.assertTrue(self.ui.model.frozen)
            self.assertIsNone(self.ui.serial)
        finally: os.close(master); os.close(slave)
if __name__=='__main__': unittest.main()
