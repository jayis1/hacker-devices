# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'app'))
import unittest
from unittest.mock import patch
import os
import pty
import termios
import tempfile
import tkinter as tk
from console import Model, Console, Serial, frame
from contract import FIELDS, validate

def row(**changes):
    s={k:0 for k in FIELDS}
    s.update(now=100,valid=15,ready=1,observed=90,baselines=1)
    s.update(changes)
    validate(s)
    return s

def send(m,s):
    m.feed(frame('S',[s[k] for k in FIELDS]))

def model(origin='replay'):
    m=Model('explicit synthetic data',origin)
    m.connected=origin=='live'
    m.feed(frame('P',[3,1,2,0]))
    return m

class IntegrityTests(unittest.TestCase):
    def test_incompatible_policy(self):
        m=model(); m.feed(frame('P',[3,1,2,1])); send(m,row())
        self.assertIsNone(m.latest); self.assertIn('Unsupported',m.frozen)
    def test_duplicate_contradiction(self):
        m=model(); send(m,row()); send(m,row(observed=91))
        self.assertIn('duplicate',m.frozen)
    def test_halted_immutable(self):
        m=model(); send(m,row(ready=0,valid=0,halted=1,faults=16))
        send(m,row(now=200,ready=0,valid=0,halted=1,faults=16,observed=91))
        self.assertIn('Halted',m.frozen)
    def test_two_simultaneous_admissions_per_tick(self):
        m=model(); send(m,row())
        send(m,row(now=101,levels=15,observed=91,starts=2,collided=2,
                   high=1,low=1,overlaps=1,overlap_ticks=1,collision_active=1,faults=4))
        self.assertFalse(m.frozen); self.assertEqual(m.latest['starts'],2)
    def test_impossible_growth(self):
        m=model(); send(m,row(starts=1,accepted=1,low=1))
        send(m,row(now=200,observed=190,starts=2,accepted=1,inversions=1,low=1,high=1,faults=2))
        self.assertIn('without low',m.frozen)
    def test_crc_valid_impossible_masks(self):
        for field,value in [('valid',16),('faults',64),('ready',2),('inversions',1),('baselines',3)]:
            m=model(); s=row(); s[field]=value; send(m,s)
            self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_slow_backlog(self):
        m=model('live')
        for i in range(300):
            with patch('console.time.monotonic',return_value=i*.02):
                send(m,row(now=100+i,observed=90+i))
        self.assertIn('progress diverged',m.frozen)
    def test_serial_restore_failed_setup(self):
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush',side_effect=OSError('injected setup failure')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
        finally: os.close(master); os.close(slave)
    def test_empty_export_and_replay_recovery_ui(self):
        root=tk.Tk(); app=Console(root)
        try:
            with tempfile.TemporaryDirectory() as directory:
                path=Path(directory)/'fixture.txt'
                s=row(); path.write_bytes(frame('P',[3,1,2,0])+frame('S',[s[k] for k in FIELDS]))
                app.load(path); root.update()
                previous=app.model
                app.path.set(str(Path(directory)/'missing')); app.connect()
                self.assertIs(app.model,previous)
                app.disconnect(); self.assertEqual(app.model.origin,'replay')
                self.assertIn('starts',app.summary.get('1.0','end'))
                empty=Model('empty','none'); report=Path(directory)/'empty.json'; empty.export(report)
                self.assertIn('"latest": null',report.read_text())
                with self.assertRaises(ValueError): empty.finish()
        finally: app.close(); root.destroy()
if __name__=='__main__': unittest.main()
