# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
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
import unittest
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'app'))
from console import Console, Model, Serial, frame, read_replay
from contract import FIELDS

class Tests(unittest.TestCase):
    def setUp(self):
        self.data = subprocess.check_output([ROOT/'firmware/build/fixture'])

    def model(self):
        m = Model('synthetic C fixture', 'replay')
        m.feed(self.data); m.finish()
        self.assertEqual(m.errors, 0)
        self.assertFalse(m.frozen)
        return m

    def test_fixture(self):
        m = self.model()
        self.assertEqual(len(m.history), 6)
        self.assertEqual(m.latest['state'], 2)
        self.assertEqual(m.latest['captured'], 0)
        self.assertTrue(m.latest['t0'] and m.latest['t1'])
        self.assertFalse(m.live())

    def test_impossible(self):
        m = self.model()
        for key, value in [('state',3), ('captured',4), ('seals',99), ('resets',0),
                           ('t0',0), ('coverage',0), ('mismatch',1), ('unsealed',1)]:
            s = dict(m.latest); s[key] = value
            other = Model('invalid', 'replay')
            other.feed(frame('P',[3,1,1]))
            other.feed(frame('S',[s[k] for k in FIELDS]))
            self.assertEqual(other.errors, 1, key)

    def test_reset_between_sparse_reports(self):
        data = subprocess.check_output([ROOT/'firmware/build/fixture','reset'])
        m = Model('synthetic sparse reset','replay')
        m.feed(data); m.finish()
        self.assertFalse(m.frozen)
        self.assertEqual(m.errors,0)
        self.assertEqual(m.latest['state'],0)
        self.assertEqual(m.latest['resets'],2)
        self.assertEqual(m.latest['seals'],1)

    def test_duplicate_and_rollback(self):
        m = self.model(); old = m.received
        last = self.data.splitlines(keepends=True)[-1]
        m.feed(last)
        self.assertEqual(m.received, old)
        s = dict(m.latest); s['tx_fail'] += 1
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertIn('duplicate',m.frozen)
        m = self.model(); s = dict(m.latest); s['now'] = 0
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertIn('rollback',m.frozen)

    def test_capture_cannot_change_without_new_seal(self):
        m = self.model(); s = dict(m.latest)
        s.update(now=s['now']+100,captured=1,levels=5)
        m.feed(frame('S',[s[k] for k in FIELDS]))
        self.assertIn('capture',m.frozen)

    def test_framing_and_eof(self):
        m = Model('synthetic','replay')
        m.feed(b'garbageSF1,S,0'+self.data)
        m.finish()
        self.assertGreater(m.errors,0)
        self.assertIsNotNone(m.latest)
        m.feed(b'SF1,S,'); before=m.errors; m.finish()
        self.assertEqual(m.errors,before+1)
        with self.assertRaises(ValueError): Model('empty','replay').finish()

    def test_private_export(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'report.json'; m=self.model(); m.export(path)
            self.assertEqual(path.stat().st_mode & 0o777,0o600)
            self.assertEqual(json.loads(path.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(path)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            big=Path(d)/'big'; big.write_bytes(b'X'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(big)

    def test_live_pause(self):
        m=Model('synthetic','live'); m.connected=True
        lines=self.data.splitlines(keepends=True)
        with patch('console.time.monotonic', return_value=0): m.feed(b''.join(lines[:3]))
        with patch('console.time.monotonic', return_value=.1): m.feed(lines[3]); self.assertTrue(m.live())
        with patch('console.time.monotonic', return_value=5): m.feed(lines[4]); self.assertFalse(m.live())
        self.assertIn('pause',m.frozen)

    def test_serial_constructor_cleanup(self):
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(old,termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)

    def test_gui_real_pty(self):
        root=tk.Tk(); app=Console(root)
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            owned=app.port; app.connect(); self.assertIs(app.port,owned)
            for line in self.data.splitlines(keepends=True):
                os.write(master,line)
                deadline=time.monotonic()+.10
                while time.monotonic()<deadline: root.update(); time.sleep(.005)
            self.assertEqual(app.model.errors,0)
            self.assertEqual(len(app.model.history),6)
            self.assertIn('SEALED',app.summary.get('1.0','end'))
            self.assertIn('captured',app.history.get('1.0','end'))
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            self.assertEqual(app.model.origin,'live'); self.assertFalse(app.model.live())
            app.connect(); os.close(master); master=None
            deadline=time.monotonic()+.1
            while time.monotonic()<deadline: root.update(); time.sleep(.005)
            self.assertIsNone(app.port)
            with tempfile.TemporaryDirectory() as d:
                path=Path(d)/'fixture'; path.write_bytes(self.data)
                app.load(path); self.assertEqual(app.model.origin,'replay')
                self.assertEqual(len(app.model.history),6)
        finally:
            app.close(); root.destroy()
            if master is not None: os.close(master)
            os.close(slave)

if __name__ == '__main__': unittest.main()
