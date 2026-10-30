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
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'app'))
from console import Model, Console, Serial, FIELDS, frame, read_replay
from contract import validate, transition, queue

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
    def test_sparse_valid_list_histories(self):
        checked=0
        def visit(q, s, ancestors, remaining):
            nonlocal checked
            validate(s)
            for a in ancestors:
                self.assertFalse(transition(a,s,s['now']-a['now']))
                checked+=1
            if not remaining: return
            choices=[(True,t) for t in range(4) if t not in q]
            if q: choices.append((False,q[0]))
            for add,tag in choices:
                nq=q+[tag] if add else q[1:]
                t=dict(s, now=s['now']+60, observed=s['observed']+60,
                       attempts=s['attempts']+1,
                       active=sum(1<<x for x in nq),
                       order=sum(x<<(2*i) for i,x in enumerate(nq)),
                       peak=max(s['peak'],len(nq)))
                if add:
                    t['launched']+=1; t['queued']+=bool(q)
                else:
                    t['completed']+=1; t['rounds']+=not nq
                visit(nq,t,ancestors+[s],remaining-1)
        visit([],snapshot(),[],7)
        self.assertGreater(checked,10000)
    def test_baseline(self):
        validate(snapshot()); self.assertEqual(queue(snapshot()),[])
    def test_impossible_queue(self):
        for change in [dict(active=3,order=0,launched=2,attempts=2,queued=1,peak=2),
                       dict(active=1,order=1,launched=1,attempts=1,peak=1),
                       dict(order=64), dict(peak=5),
                       dict(launched=2,completed=2,attempts=4,rounds=1,queued=1,peak=1)]:
            with self.assertRaises(ValueError): validate(snapshot(**change))
    def test_conservation(self):
        for key in ('launched','rounds','completed','attempts','rejected','queued','peak'):
            with self.assertRaises(ValueError): validate(snapshot(**{key:1}))
    def test_no_silent_reorder(self):
        a=snapshot(active=5,order=2,launched=2,attempts=2,queued=1,peak=2)
        b=dict(a,now=200,order=8)
        validate(a); validate(b)
        self.assertTrue(transition(a,b,100))
        m=model(); m.feed(status(a)); m.feed(status(b)); self.assertTrue(m.frozen)
    def test_head_release_and_reentry(self):
        a=snapshot(active=5,order=2,launched=2,attempts=2,queued=1,peak=2)
        b=dict(a,now=200,order=8,launched=3,completed=1,attempts=4,queued=2)
        validate(a); validate(b); self.assertFalse(transition(a,b,100))
    def test_new_head_cannot_overtake_survivor(self):
        a=snapshot(active=5,order=2,launched=2,attempts=2,queued=1,peak=2)
        b=dict(a,now=200,order=2,launched=3,completed=1,attempts=4,queued=2)
        validate(b); self.assertTrue(transition(a,b,100))
    def test_empty_boundary(self):
        a=snapshot(active=1,launched=1,attempts=1,peak=1)
        b=snapshot(now=200,rounds=1,active=8,order=3,launched=2,completed=1,attempts=3,peak=1)
        validate(a); validate(b); self.assertFalse(transition(a,b,100))
    def test_invalid_crc_valid_record(self):
        m=model(); m.feed(status(snapshot(launched=1)))
        self.assertEqual(m.errors,1); self.assertIsNone(m.latest)
    def test_wrong_protocol_and_policy(self):
        m=Model('wrong','replay'); m.feed(b'RS1,P,3,1,5,4*0000\n')
        self.assertFalse(m.policy); self.assertEqual(m.errors,1)
        m.feed(frame('P',[3,1,6,4])); self.assertTrue(m.frozen)
    def test_duplicate_timestamp_does_not_refresh(self):
        m=model('live'); m.connected=True
        with patch('console.time.monotonic',return_value=0): m.feed(status(snapshot()))
        with patch('console.time.monotonic',return_value=.1): m.feed(status(snapshot(now=200)))
        with patch('console.time.monotonic',return_value=1.2):
            m.feed(status(snapshot(now=200))); self.assertFalse(m.live())
    def test_pause_and_gradual_backlog(self):
        m=model('live')
        with patch('console.time.monotonic',return_value=0): m.feed(status(snapshot()))
        with patch('console.time.monotonic',return_value=2): m.feed(status(snapshot(now=200)))
        self.assertIn('pause',m.frozen)
        m=model('live')
        for i in range(30):
            with patch('console.time.monotonic',return_value=i*.1): m.feed(status(snapshot(now=100+i)))
        self.assertIn('diverged',m.frozen)
    def test_rollback_halt_and_boot(self):
        m=model(); m.feed(status(snapshot(now=1000))); m.feed(status(snapshot(now=900)))
        self.assertIn('rollback',m.frozen)
        m=model(); s=snapshot(halted=1,valid=0,faults=2,rejected=1,attempts=1)
        m.feed(status(s)); s.update(now=200,observed=91); m.feed(status(s))
        self.assertIn('Halted',m.frozen)
        m=model(); m.feed(status(snapshot())); m.feed(frame('B',[42]))
        self.assertEqual(m.later_reset,42); self.assertIn('Boot',m.frozen)
    def test_resynchronization_and_eof(self):
        m=model(); m.feed(b'QO1,S,0'+status(snapshot())); m.feed(b'QO1,S'); m.finish()
        self.assertEqual(m.errors,2); self.assertIsNotNone(m.latest)
        with self.assertRaises(ValueError): model().finish()
    def test_replay_and_private_export(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=Model('C encoder fixture','replay'); m.feed(data); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(m.latest['rounds'],1); self.assertEqual(m.latest['faults'],1)
        self.assertEqual(queue(m.latest),[3,1]); self.assertFalse(m.live())
        with tempfile.TemporaryDirectory() as d:
            output=Path(d)/'report.json'; m.export(output)
            self.assertEqual(output.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(output.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(output)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(large)
    def test_serial_constructor_restores_attributes(self):
        master,slave=pty.openpty()
        try:
            old=termios.tcgetattr(slave)
            with patch('console.termios.tcflush',side_effect=OSError('injected failure')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),old)
        finally:
            os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def setUp(self):
        self.root=tk.Tk(); self.ui=Console(self.root)
        self.master,self.slave=pty.openpty()
        self.old=termios.tcgetattr(self.slave)
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
            if line.startswith(b'QO1,S,'):
                now=int(line.split(b',')[2]); self.pump(max(.001,(now-previous)/1000)); previous=now
            os.write(self.master,line); self.pump(.002)
        self.pump()
        self.assertEqual((self.ui.model.errors,self.ui.model.frozen),(0,''))
        self.assertTrue(self.ui.model.live()); self.assertEqual(self.ui.model.latest['rounds'],1)
        self.assertIn('HALTED',self.ui.summary.get('1.0','end'))
        self.assertIn('[3, 1]',self.ui.summary.get('1.0','end'))
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
    def test_replay_replaces_view_and_restores_serial(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'capture'; path.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
            self.ui.load(path); self.pump()
            self.assertEqual(self.ui.model.origin,'replay'); self.assertFalse(self.ui.model.connected)
            self.assertIn('rounds: 1',self.ui.summary.get('1.0','end'))
            self.assertEqual(termios.tcgetattr(self.slave),self.old)
    def test_late_attach_suffix(self):
        os.write(self.master,b'123*0000\n'+frame('P',[3,1,5,4])+status(snapshot()))
        self.pump()
        self.assertIsNotNone(self.ui.model.latest); self.assertEqual(self.ui.model.errors,1)
if __name__=='__main__': unittest.main()
