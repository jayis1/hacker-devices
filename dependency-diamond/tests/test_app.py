# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import json
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from console import Model, Console, FIELDS, frame, read_replay
from contract import validate, transition

def snapshot(**changes):
    s=dict.fromkeys(FIELDS,0)
    s.update(now=100,valid=15,phase=1,stable=5,observed=90)
    s.update(changes)
    return s

def status(s):
    return frame('S',[s[k] for k in FIELDS])

def model(origin='replay'):
    m=Model('synthetic',origin); m.feed(frame('P',[3,1,5,4])); return m

class ContractTests(unittest.TestCase):
    def test_baseline(self):
        validate(snapshot())
    def test_impossible_graph(self):
        for change in [dict(active=8,launched=1,attempts=1),
                       dict(active=1,done=1,launched=1,completed=1,attempts=2),
                       dict(done=2,launched=1,completed=1,attempts=2)]:
            with self.assertRaises(ValueError): validate(snapshot(**change))
    def test_conservation(self):
        for change in [dict(launched=1),dict(rounds=1),dict(completed=1),dict(attempts=1),dict(rejected=1)]:
            with self.assertRaises(ValueError): validate(snapshot(**change))
    def test_sibling_swap_cannot_preserve_counts(self):
        a=snapshot(active=2,done=1,launched=2,completed=1,attempts=3)
        b=snapshot(now=200,active=4,done=1,launched=2,completed=1,attempts=3)
        validate(a); validate(b); self.assertTrue(transition(a,b,100))
    def test_completion_and_restart_boundary(self):
        a=snapshot(active=8,done=7,launched=4,completed=3,attempts=7)
        b=snapshot(now=200,rounds=1,active=1,launched=5,completed=4,attempts=9)
        validate(a); validate(b); self.assertFalse(transition(a,b,100))
    def test_invalid_crc_valid_record(self):
        m=model(); m.feed(status(snapshot(launched=1)))
        self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_wrong_protocol_and_policy(self):
        m=Model('wrong','replay'); m.feed(b'PR1,P,3,1,5,4*0000\n')
        self.assertFalse(m.policy); self.assertEqual(m.errors,1)
        m.feed(frame('P',[3,1,6,4])); self.assertTrue(m.frozen)
    def test_duplicate_timestamp_does_not_refresh(self):
        m=model('live'); m.connected=True
        with patch('console.time.monotonic',return_value=0): m.feed(status(snapshot()))
        with patch('console.time.monotonic',return_value=.1): m.feed(status(snapshot(now=200)))
        with patch('console.time.monotonic',return_value=1.2):
            m.feed(status(snapshot(now=200))); self.assertFalse(m.live())
    def test_pause_freezes(self):
        m=model('live')
        with patch('console.time.monotonic',return_value=0): m.feed(status(snapshot()))
        with patch('console.time.monotonic',return_value=2): m.feed(status(snapshot(now=200)))
        self.assertIn('pause',m.frozen)
    def test_rollback_and_halt(self):
        m=model(); m.feed(status(snapshot(now=1000))); m.feed(status(snapshot(now=900)))
        self.assertIn('rollback',m.frozen)
        m=model(); s=snapshot(halted=1,valid=0,faults=2,rejected=1,attempts=1)
        m.feed(status(s)); s.update(now=200,observed=91); m.feed(status(s))
        self.assertIn('Halted',m.frozen)
    def test_resynchronization_and_eof(self):
        m=model(); m.feed(b'DD1,S,0'+status(snapshot())); m.feed(b'DD1,S'); m.finish()
        self.assertEqual(m.errors,2); self.assertIsNotNone(m.latest)
    def test_replay_and_private_export(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=Model('C encoder fixture','replay'); m.feed(data); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(m.latest['rounds'],1); self.assertEqual(m.latest['faults'],1)
        self.assertFalse(m.live())
        with tempfile.TemporaryDirectory() as d:
            output=Path(d)/'report.json'; m.export(output)
            self.assertEqual(output.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(output.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(output)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Console(self.root)
        self.master,self.slave=pty.openpty()
        self.ui.path.set(os.ttyname(self.slave)); self.ui.connect()
        self.assertTrue(self.ui.model.connected)
    def tearDown(self):
        self.ui.close(); self.root.destroy()
        if self.master is not None: os.close(self.master)
        os.close(self.slave)
    def pump(self,seconds=.05):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            self.root.update(); time.sleep(.005)
    def test_live_screens_export_disconnect(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        previous=0
        for line in data.splitlines(keepends=True):
            if line.startswith(b'DD1,S,'):
                now=int(line.split(b',')[2]); self.pump(max(.001,(now-previous)/1000)); previous=now
            os.write(self.master,line); self.pump(.002)
        self.pump()
        self.assertEqual((self.ui.model.errors,self.ui.model.frozen),(0,''))
        self.assertTrue(self.ui.model.live()); self.assertEqual(self.ui.model.latest['rounds'],1)
        self.assertIn('HALTED',self.ui.summary.get('1.0','end'))
        self.assertIn('completed',self.ui.history.get('1.0','end'))
        for i in range(3): self.ui.tabs.select(i); self.pump()
        with tempfile.TemporaryDirectory() as d:
            target=Path(d)/'export.json'
            with patch('console.filedialog.asksaveasfilename',return_value=str(target)): self.ui.export()
            report=json.loads(target.read_text()); self.assertEqual(report['latest']['faults'],1)
            self.assertEqual(report['origin'],'live')
        port=self.ui.port; self.ui.connect(); self.assertIs(self.ui.port,port)
        os.close(self.master); self.master=None; self.pump()
        self.assertFalse(self.ui.model.connected); self.assertFalse(self.ui.model.live())
        self.assertEqual(self.ui.model.report()['origin'],'live')
    def test_replay_replaces_view(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'capture'; path.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
            self.ui.load(path); self.pump()
            self.assertEqual(self.ui.model.origin,'replay'); self.assertFalse(self.ui.model.connected)
            self.assertIn('rounds: 1',self.ui.summary.get('1.0','end'))
if __name__=='__main__': unittest.main()
