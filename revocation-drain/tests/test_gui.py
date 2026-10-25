# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import os
import pty
import subprocess
import tempfile
import time
import tkinter as tk
import unittest
from pathlib import Path
from unittest.mock import patch
from test_app import app, root, frame

class GuiTests(unittest.TestCase):
    def test_actual_widgets_pty_disconnect_replay_export(self):
        top=tk.Tk(); w=app.Window(top)
        master,slave=pty.openpty()
        try:
            w.authorized.set(True)
            w.port.delete(0,'end'); w.port.insert(0,os.ttyname(slave)); w.connect()
            fixture=subprocess.check_output([root/'firmware/build/fixture'])
            os.write(master,fixture)
            end=time.monotonic()+0.3
            while time.monotonic()<end:
                top.update(); time.sleep(0.01)
            self.assertTrue(w.model.connected)
            self.assertEqual(w.model.latest['completed'],1)
            self.assertIn('Acknowledgments after quiet: 1',w.summary.get('1.0','end'))
            self.assertEqual(len(w.history.get_children()),len(w.model.rows))
            w.model.arrival=time.monotonic()-2; w.render()
            self.assertIn('STALE',w.status.get())
            os.close(master); master=None
            end=time.monotonic()+0.15
            while time.monotonic()<end:
                top.update(); time.sleep(0.01)
            self.assertFalse(w.model.connected)
            self.assertTrue(w.model.source.startswith('Live'))
            with tempfile.TemporaryDirectory() as td:
                target=Path(td)/'report.json'
                with patch.object(app.filedialog,'asksaveasfilename',return_value=str(target)):
                    w.save()
                self.assertIn('jayis1',target.read_text())
                replay=Path(td)/'fixture.txt'; replay.write_bytes(fixture)
                with patch.object(app.filedialog,'askopenfilename',return_value=str(replay)):
                    w.replay()
                self.assertTrue(w.model.source.startswith('Replay'))
                self.assertEqual(w.model.latest['completed'],1)
                replay.write_bytes(frame('P',20,500)+frame('S',1,1,0,0,0,0,0,0,0,0,0,0,0,0))
                w.load(replay)
                self.assertIn('Acknowledgments after quiet: 0',w.summary.get('1.0','end'))
                self.assertEqual(len(w.history.get_children()),1)
                fifo=Path(td)/'fifo'; os.mkfifo(fifo)
                with patch.object(app.filedialog,'askopenfilename',return_value=str(fifo)), patch.object(app.messagebox,'showerror') as error:
                    w.replay(); error.assert_called_once()
        finally:
            if master is not None: os.close(master)
            os.close(slave); w.close()

    def test_constructor_restores_terminal_on_failure(self):
        master,slave=pty.openpty()
        try:
            saved=app.termios.tcgetattr(slave)
            with patch.object(app.termios,'tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): app.Serial(os.ttyname(slave))
            self.assertEqual(saved,app.termios.tcgetattr(slave))
        finally:
            os.close(master); os.close(slave)

if __name__=='__main__': unittest.main()
