# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
"""Exercise real Tk, Linux serial PTY, and production C-generated frames."""
import json
import os
from pathlib import Path
import pty
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
from test_app import app, fixture, packet

class GUI(unittest.TestCase):
    def pump(self, seconds=.1):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update()
            time.sleep(.005)

    def setUp(self):
        self.root=tk.Tk()
        self.window=app.Window(self.root)
        self.master,self.slave=pty.openpty()
        self.window.port.set(os.ttyname(self.slave))

    def tearDown(self):
        self.window.close()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)

    def test_screens_replay_export_disconnect_reconnect(self):
        assert self.master is not None
        w=self.window
        w.connect()
        self.assertTrue(w.model.connected)
        lines=fixture().splitlines(keepends=True)
        # Pace actual encoder snapshots at their nominal 100ms cadence.
        os.write(self.master,b''.join(lines[:3])); self.pump(.1)
        for line in lines[3:]:
            os.write(self.master,line); self.pump(.1)
        self.assertEqual(w.model.errors,0)
        self.assertFalse(w.model.frozen)
        self.assertEqual(w.model.status['unsafe'],1)
        self.assertTrue(w.model.fresh(time.monotonic()))
        for i,name in enumerate(w.views):
            w.tabs.select(i); self.pump(.02)
            self.assertTrue(w.views[name].get('1.0','end').strip())
        self.assertIn('unsafe: 1',w.views['Permission'].get('1.0','end'))
        self.assertEqual(len(w.model.history),11)
        with tempfile.TemporaryDirectory() as d:
            dest=Path(d)/'report.json'
            with patch.object(app.filedialog,'asksaveasfilename',return_value=str(dest)):
                w.export_dialog()
                with patch.object(app.messagebox,'showerror') as error:
                    w.export_dialog(); error.assert_called_once()
            self.assertEqual(json.loads(dest.read_text())['author'],'jayis1')
            self.assertEqual(dest.stat().st_mode & 0o777,0o600)
            os.close(self.master); self.master=None; self.pump()
            self.assertFalse(w.model.connected)
            self.assertEqual(w.model.report(time.monotonic())['provenance'],'live serial')
            replay=Path(d)/'synthetic.txt'; replay.write_bytes(fixture())
            w.replay(replay)
            self.assertEqual(w.model.errors,0)
            self.assertFalse(w.model.fresh(time.monotonic()))
            self.assertEqual(len(w.model.history),11)
            self.master,newslave=pty.openpty()
            w.port.set(os.ttyname(newslave)); w.connect(); self.pump()
            self.assertIsNone(w.model.status)
            self.assertNotIn('unsafe: 1',w.views['Permission'].get('1.0','end'))
            os.close(newslave)

    def test_permission_failure_and_constructor_cleanup(self):
        w=self.window
        with patch.object(app.os,'open',side_effect=PermissionError('denied')):
            w.connect()
        self.assertFalse(w.model.connected)
        self.assertIn('denied',w.model.frozen)
        original=termios.tcgetattr(self.slave)
        with patch.object(app.termios,'tcflush',side_effect=termios.error('injected')):
            with self.assertRaises(termios.error): app.Serial(os.ttyname(self.slave))
        self.assertEqual(termios.tcgetattr(self.slave),original)

    def test_host_pause_does_not_make_backlog_live(self):
        assert self.master is not None
        w=self.window; w.connect()
        lines=fixture().splitlines(keepends=True)
        os.write(self.master,b''.join(lines[:3])); self.pump(.1)
        self.pump(1.1)
        os.write(self.master,lines[3]); self.pump(.1)
        self.assertTrue(w.model.frozen)
        self.assertFalse(w.model.fresh(time.monotonic()))

if __name__=='__main__': unittest.main()
