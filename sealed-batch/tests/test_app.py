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
from contract import FIELDS, POLICY, validate, transition, eligible

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def snapshot(ops):
    # This builder intentionally permits impossible policy histories.
    s=baseline()
    for tag,alloc in ops:
        s['now']+=24; s['observed']+=24; s['attempts']+=1
        if alloc:
            s['queued']+=bool(s['active']); s['active']|=1<<tag
            s['launched']+=1; s['use'+str(tag)]+=1; s['stamp'+str(tag)]=s['launched']
            s['depth']+=1; s['peak']=max(s['peak'],s['depth'])
            s['batch_size']+=1
        else:
            s['active']&=~(1<<tag); s['depth']-=1; s['completed']+=1
            s['rounds']+=not s['active']
            if not s['active']: s['batch_size']=0
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

class ContractTests(unittest.TestCase):
    def test_all_masks_and_phases(self):
        for mask in range(16):
            size=mask.bit_count()
            self.assertEqual(eligible({'active':mask,'batch_size':size}),[i for i in range(4) if not mask&(1<<i)])
            if 0<size<4:
                self.assertEqual(eligible({'active':mask,'batch_size':size+1}),[])
    def test_legal_sparse_prefix_pairs(self):
        def visit(ops,members,draining,ancestors,depth):
            b=snapshot(ops); validate(b)
            for a in ancestors:
                self.assertEqual(transition(a,b,b['now']-a['now']),'',(ops,a,b))
            if not depth: return
            for tag in range(4):
                alloc=tag not in members
                if alloc and draining: continue
                new=members|{tag} if alloc else members-{tag}
                nextdrain=draining if alloc else bool(new)
                visit(ops+[(tag,alloc)],new,nextdrain,ancestors+[b],depth-1)
        visit([],set(),False,[],7)
    def test_c_fixtures(self):
        for args,active,completed,fault in [([],2,5,1),(['legal'],0,6,0),(['repeat'],0,3,0)]:
            m=Model('synthetic C encoder','replay')
            m.feed(subprocess.check_output([ROOT/'firmware/build/fixture',*args])); m.finish()
            self.assertEqual((m.errors,m.frozen),(0,''))
            self.assertEqual((m.latest['active'],m.latest['completed'],m.latest['faults']),(active,completed,fault))
    def test_seal_cannot_disappear(self):
        ops=[(0,True),(1,True),(0,False)]; a=snapshot(ops)
        b=dict(a,now=a['now']+100,observed=a['observed']+100,batch_size=1)
        validate(b)
        m=Model('CRC-valid impossible seal removal','replay'); m.feed(stream(a,b))
        self.assertIn('batch size',m.frozen)
    def test_sealed_hidden_admission(self):
        ops=[(0,True),(1,True),(0,False)]; a=snapshot(ops)
        b=snapshot(ops+[(2,True),(2,False)]); b['peak']=3; validate(b)
        self.assertIn('Sealed batch',transition(a,b,48))
    def test_no_batch_close_around_continuous_incumbent(self):
        history=[(2,True),(3,True),(2,False),(3,False)]
        a=snapshot(history+[(0,True),(1,True)])
        b=snapshot(history+[(0,True),(1,True),(1,False)])
        # Move cohort accounting while retaining all global conservation.
        b.update(rounds=2,queued=1,batch_size=1)
        validate(b)
        self.assertIn('persistent resident',transition(a,b,24))
    def test_complete_drain_allows_reuse(self):
        a=snapshot([(0,True),(1,True),(0,False)])
        b=snapshot([(0,True),(1,True),(0,False),(1,False),(1,True)])
        self.assertEqual(transition(a,b,48),'')
    def test_large_gap_legal_batches(self):
        ops=[]; a=baseline()
        for _ in range(12): ops.extend([(0,True),(1,True),(0,False),(1,False)])
        b=snapshot(ops); validate(b)
        self.assertEqual(transition(a,b,b['now']-a['now']),'')
    def test_completed_batch_peak_cannot_be_underreported(self):
        a=baseline()
        b=snapshot([(0,True),(1,True),(2,True),(0,False),(1,False),(2,False)])
        self.assertEqual(transition(a,b,b['now']-a['now']),'')
        b['peak']=2
        validate(b)
        m=Model('CRC-valid impossible batch peak','replay'); m.feed(stream(a,b))
        self.assertTrue(m.frozen)
    def test_fault_context_and_invalid_counts(self):
        for s,fault in [(baseline(),1),(baseline(),4),(snapshot([(0,True)]),2)]:
            s.update(halted=1,valid=0,faults=fault,rejected=1,attempts=s['attempts']+1)
            with self.assertRaises(ValueError): validate(s)
        for k,v in [('active',1),('use0',1),('depth',1),('peak',5),('rounds',1),('faults',3),('now',True),('batch_size',1)]:
            s=baseline(); s[k]=v
            with self.assertRaises(ValueError): validate(s)
    def test_parser_recovery_crc_tail_and_foreign_protocol(self):
        m=Model('synthetic malformed','replay')
        m.feed(b'x'*900+b'SB1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        n=m.errors; m.feed(b'SB1,S,1*0000\n'); self.assertGreater(m.errors,n)
        m.feed(b'SB1,S'); n=m.errors; m.finish(); self.assertEqual(m.errors,n+1)
        m=Model('foreign','replay'); m.feed(stream(baseline()).replace(b'SB1,',b'DF1,'))
        self.assertFalse(m.policy); self.assertIsNone(m.latest)
    def test_policy_reset_and_halt_freeze(self):
        m=Model('bad policy','replay'); m.feed(frame('P',[3,1,5,4,0,1])); self.assertTrue(m.frozen)
        m=Model('reset','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
        a=snapshot([(0,True),(1,True),(0,False)])
        a.update(halted=1,valid=0,faults=1,rejected=1,attempts=4,levels=2)
        b=dict(a,now=a['now']+100,observed=a['observed']+100)
        m=Model('halt','replay'); m.feed(stream(a,b)); self.assertIn('Halted evidence',m.frozen)
    def test_live_stale_duplicate_and_drift(self):
        m=Model('synthetic live','live'); m.connected=True
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
    def test_private_export_replay_bounds_and_termios_restore(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'export.json'; m=Model('test','replay'); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            with patch('console.os.open',side_effect=OSError('disk full')):
                with self.assertRaises(OSError): m.export(Path(d)/'full.json')
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
    def test_permission_denial_retains_evidence(self):
        root=tk.Tk(); app=Console(root)
        try:
            app.model.feed(stream(baseline())); retained=app.model
            with patch('console.os.open',side_effect=PermissionError('permission denied')): app.connect()
            root.update(); app.draw()
            self.assertIs(app.model,retained); self.assertIsNone(app.port)
            self.assertIn('permission denied',app.integrity.get('1.0','end'))
        finally: app.close(); root.destroy()
    def test_screens_pty_disconnect_replay_export(self):
        root=tk.Tk(); app=Console(root); master,slave=pty.openpty(); old=termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            os.write(master,stream(snapshot([(0,True),(1,True),(0,False)])))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertIn('Eligible allocation tags: []',app.summary.get('1.0','end'))
            self.assertIn('Batch: DRAINING',app.summary.get('1.0','end'))
            self.assertEqual(len(app.tabs.tabs()),3)
            for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
            app.connect(); self.assertIn('Disconnect',app.message)
            app.disconnect(); self.assertEqual(old,termios.tcgetattr(slave))
            app.connect(); os.close(master); master=None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'fixture'; p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture']))
                app.load(p); root.update()
                self.assertEqual(app.model.latest['faults'],1)
                self.assertIn('admission after batch began draining',app.summary.get('1.0','end'))
                self.assertEqual(len(app.model.history),13)
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
                self.assertEqual(app.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                saved=json.loads(out.read_text()); self.assertEqual(saved['latest']['active'],2)
                self.assertEqual(saved['author'],'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
