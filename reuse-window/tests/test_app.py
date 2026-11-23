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
from console import Console, Model, frame, read_replay, Serial
from contract import FIELDS, POLICY, validate, transition, eligible, timers

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

def fixture(mode=None):
    args=[ROOT/'firmware/build/fixture']
    if mode: args.append(mode)
    return subprocess.check_output(args)

def states(data):
    result=[]
    for line in data.splitlines():
        if line.startswith(b'WN1,S,'):
            result.append(dict(zip(FIELDS,map(int,line.split(b'*')[0].split(b',')[2:]))))
    return result

class ContractTests(unittest.TestCase):
    def test_c_fixture(self):
        m=Model('synthetic C fixture','replay'); m.feed(fixture()); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual((m.latest['faults'],m.latest['launched'],m.latest['completed']),(1,8,8))
        self.assertEqual(m.latest['active'],0)
    def test_dense_and_sparse_real_encoder(self):
        data=fixture('dense'); all_states=states(data)
        m=Model('dense synthetic fixture','replay'); m.feed(data); m.finish()
        self.assertEqual((m.errors,m.frozen),(0,''))
        self.assertEqual(m.latest['faults'],16)
        # Exact adjacent checks plus deterministic sparse histories spanning
        # startup, replenishment, simultaneous timers, renewal and final gap.
        for stride in (1,2,7,31,101,503):
            chosen=all_states[::stride]
            if chosen[-1]!=all_states[-1]: chosen.append(all_states[-1])
            m=Model('sparse synthetic fixture','replay'); m.feed(stream(*chosen)); m.finish()
            self.assertEqual((m.errors,m.frozen),(0,''),stride)
        print('dense C-encoder snapshots:',len(all_states))
    def test_eligibility_and_deadlines(self):
        for s in states(fixture('dense')):
            validate(s)
            self.assertEqual(eligible(s),[i for i,(old,new) in enumerate(timers(s))
                                         if not old and not s['active']&(1<<i)])
    def test_deadline_without_admission(self):
        a=states(fixture())[2] # first return, existing newest deadline
        b=dict(a,now=a['now']+1,observed=a['observed']+1)
        # CRC-valid, individually valid, but timer failed to decrease.
        validate(b); m=Model('forged timer','replay'); m.feed(stream(a,b))
        self.assertIn('Deadline changed',m.frozen)
    def test_previous_deadline_cannot_migrate(self):
        seq=states(fixture()); a=seq[8]; b=dict(seq[9])
        self.assertEqual(b['use0']-a['use0'],1)
        b['old0']-=1; validate(b)
        m=Model('forged prior deadline','replay'); m.feed(stream(a,b))
        self.assertIn('Previous newest',m.frozen)
    def test_hidden_third_admission_blocked(self):
        seq=states(fixture()); a=seq[16]
        # Accounting permits another complete allocate/return cycle, but its
        # oldest deadline cannot expire within the proposed 48 acquired ticks.
        b=dict(a,now=a['now']+48,observed=a['observed']+48,
               attempts=a['attempts']+2,launched=9,completed=9,rounds=9,
               use0=3,stamp0=9)
        for i in range(4):
            b['old'+str(i)]=max(0,a['old'+str(i)]-48)
            b['new'+str(i)]=max(0,a['new'+str(i)]-48)
        b['old0']=a['new0']-48; b['new0']=968
        validate(b)
        m=Model('forged hidden renewal','replay'); m.feed(stream(a,b))
        self.assertIn('rolling-window capacity',m.frozen)
    def test_deadline_cannot_backdate_renewal(self):
        a=states(fixture())[16]
        acquired=a['old0']
        self.assertEqual((a['old0'],a['new0']),(632,824))
        b=dict(a,now=a['now']+acquired,observed=a['observed']+acquired,
               attempts=a['attempts']+2,launched=9,completed=9,rounds=9,
               use0=3,stamp0=9)
        for i in range(4):
            b['old'+str(i)]=max(0,a['old'+str(i)]-acquired)
            b['new'+str(i)]=max(0,a['new'+str(i)]-acquired)
        b['old0']=a['new0']-acquired
        # Total elapsed time permits renewal, but this deadline dates it to
        # offset one, long before the oldest admission actually expires.
        b['new0']=1001-acquired
        validate(b)
        m=Model('backdated renewal','replay'); m.feed(stream(a,b))
        self.assertIn('deadline predates',m.frozen)
    def test_older_deadline_cannot_backdate_renewal(self):
        a=states(fixture())[16]
        acquired=a['new0']
        b=dict(a,now=a['now']+acquired,observed=a['observed']+acquired,
               attempts=a['attempts']+4,launched=10,completed=10,rounds=10,
               use0=4,stamp0=10)
        for i in range(4):
            b['old'+str(i)]=max(0,a['old'+str(i)]-acquired)
            b['new'+str(i)]=max(0,a['new'+str(i)]-acquired)
        b['old0']=300; b['new0']=1000
        validate(b)
        m=Model('backdated penultimate renewal','replay'); m.feed(stream(a,b))
        self.assertIn('Older deadline predates',m.frozen)
    def test_gap_cannot_spend_wall_time(self):
        a=states(fixture())[2]
        b=dict(a,now=a['now']+50,halted=1,valid=0,faults=16,gaps=1,missed=49)
        b['new0']-=1; validate(b)
        m=Model('forged gap credit','replay'); m.feed(stream(a,b))
        self.assertIn('Deadline changed',m.frozen)
    def test_halted_immutable(self):
        a=states(fixture())[-1]; b=dict(a,now=a['now']+100)
        b['new0']-=1; validate(b)
        m=Model('forged terminal mutation','replay'); m.feed(stream(a,b))
        self.assertIn('Halted evidence',m.frozen)
    def test_invalid_counts_and_timers(self):
        for key,value in [('old0',1),('new0',1),('active',1),('use0',1),('depth',1),
                          ('peak',5),('rounds',1),('faults',3),('now',True),('stamp0',1)]:
            with self.subTest(key=key):
                s=baseline(); s[key]=value
                with self.assertRaises(ValueError): validate(s)
        for tag in range(4):
            s=baseline(); s.update(levels=tag,halted=1,valid=0,faults=1,rejected=1,attempts=1)
            with self.assertRaises(ValueError): validate(s)
    def test_parser_recovery_tail_policy_and_reset(self):
        m=Model('test','replay'); m.feed(b'x'*900+b'WN1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        n=m.errors; m.feed(b'WN1,S,1*0000\n'); self.assertGreater(m.errors,n)
        m.feed(b'WN1,S'); n=m.errors; m.finish(); self.assertEqual(m.errors,n+1)
        m=Model('test','replay'); m.feed(frame('P',[3,1,5,4,2,1])); self.assertTrue(m.frozen)
        m=Model('test','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
        with self.assertRaises(ValueError): Model('empty','replay').finish()
    def test_live_stale_duplicate_and_drift(self):
        m=Model('test','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(stream(baseline()))
        b=dict(baseline(),now=120,observed=117)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(stream(b)); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            m.feed(stream(b)); self.assertFalse(m.live())
            m.feed(stream(dict(b,now=220,observed=217))); self.assertTrue(m.frozen)
        m=Model('slow delivery','live'); m.connected=True
        for i in range(200):
            with patch('console.time.monotonic',return_value=10+i*.11):
                m.feed(stream(dict(baseline(),now=20+i*100,observed=17+i*100)))
            if m.frozen: break
        self.assertIn('progress diverged',m.frozen)
    def test_clock_only_progress(self):
        m=Model('false progress','replay')
        m.feed(stream(baseline(),dict(baseline(),now=120)))
        self.assertIn('omitted acquired ticks',m.frozen)
    def test_export_bounds_and_restore(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m=Model('test','replay'); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            p=Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
            p=Path(d)/'huge'; p.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(p)
        with self.assertRaises((OSError,termios.error)): Serial('/dev/null')
        master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(old,termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)

class GuiTests(unittest.TestCase):
    def test_live_replay_screens_export_and_failures(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            # Use the real encoder bytes through the real GUI serial event loop.
            data=fixture(); os.write(master,data)
            for _ in range(12): root.update(); time.sleep(.025)
            self.assertEqual((app.model.errors,app.model.frozen),(0,''))
            self.assertEqual(app.model.latest['faults'],1)
            self.assertIn('third admission inside rolling window',app.summary.get('1.0','end'))
            self.assertIn('Remaining (older, newer)',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            self.assertEqual(app.history.get('1.0','end').count("'now':"),len(app.model.history))
            app.connect(); self.assertIn('Disconnect',app.message)
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            retained=app.model
            with patch('console.os.open',side_effect=PermissionError('permission denied')): app.connect()
            self.assertIs(app.model,retained)
            app.connect(); os.close(master); master=None
            for _ in range(8): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'; p.write_bytes(fixture('legal')); app.load(p); root.update()
                self.assertEqual((app.model.errors,app.model.frozen),(0,''))
                self.assertEqual(app.model.latest['halted'],0)
                self.assertIn('Eligible allocation tags: [0, 1, 2, 3]',app.summary.get('1.0','end'))
                self.assertEqual(timers(app.model.latest),[(0,0)]*4)
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
                self.assertEqual(app.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                saved=json.loads(out.read_text()); self.assertEqual(saved['latest']['launched'],8)
                self.assertEqual(saved['author'],'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
