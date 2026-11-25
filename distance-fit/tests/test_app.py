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
from contract import FIELDS, POLICY, validate, transition, eligible, distances

def baseline():
    s=dict.fromkeys(FIELDS,0)
    s.update(now=20,valid=15,phase=1,stable=5,observed=17)
    return s

def snapshot(ops):
    # Accounting builder intentionally allows impossible policy choices:
    # validators must reject those without relying on CRC corruption.
    s=baseline()
    for tag,alloc in ops:
        s['now']+=24; s['observed']+=24; s['attempts']+=1
        if alloc:
            s['queued']+=bool(s['active']); s['active']|=1<<tag
            s['launched']+=1; s['use'+str(tag)]+=1; s['stamp'+str(tag)]=s['launched']
            s['depth']+=1; s['peak']=max(s['peak'],s['depth'])
        else:
            s['active']&=~(1<<tag); s['depth']-=1; s['completed']+=1
            s['rounds']+=not s['active']
    return s

def stream(*states):
    return frame('P',POLICY)+b''.join(frame('S',[s[k] for k in FIELDS]) for s in states)

def wavefront(mask):
    if not mask: return list(range(4))
    frontier=0
    while mask!=15:
        expanded=(mask|(mask<<1)|(mask>>1))&15
        frontier=expanded&~mask
        mask=expanded
    return [i for i in range(4) if frontier&(1<<i)]

class ContractTests(unittest.TestCase):
    def test_all_masks(self):
        for mask in range(16):
            self.assertEqual(eligible({'active':mask}),wavefront(mask))
        self.assertEqual(distances({'active':1}),{1:1,2:2,3:3})
        self.assertEqual(distances({'active':0}),dict.fromkeys(range(4)))
    def test_legal_sparse_prefix_pairs(self):
        def visit(ops,mask,ancestors,depth):
            b=snapshot(ops); validate(b)
            for a in ancestors:
                self.assertEqual(transition(a,b,b['now']-a['now']),'')
            if not depth: return
            for tag in range(4):
                alloc=not bool(mask&(1<<tag))
                if alloc and tag not in wavefront(mask): continue
                visit(ops+[(tag,alloc)],mask^(1<<tag),ancestors+[b],depth-1)
        visit([],0,[],6)
    def test_c_fixtures(self):
        for args,active,completed,fault in [([],1,5,1),(['legal'],0,5,0),(['repeat'],0,3,0)]:
            with self.subTest(args=args):
                m=Model('synthetic C encoder','replay')
                m.feed(subprocess.check_output([ROOT/'firmware/build/fixture',*args])); m.finish()
                self.assertEqual((m.errors,m.frozen),(0,''))
                self.assertEqual((m.latest['active'],m.latest['completed'],m.latest['faults']),(active,completed,fault))
    def test_persistent_resident_blocks_hidden_closer_reuse(self):
        ops=[(0,True)]; a=snapshot(ops)
        for count in [1,10]:
            b=snapshot(ops+[(1,alloc) for _ in range(count) for alloc in (True,False)])
            validate(b)
            self.assertIn('Persistent occupancy',transition(a,b,b['now']-a['now']))
    def test_farther_slot_can_fill_before_closer_admission(self):
        a=snapshot([(0,True)])
        b=snapshot([(0,True),(3,True),(1,True),(1,False)])
        validate(b); self.assertEqual(transition(a,b,b['now']-a['now']),'')
    def test_resident_can_return_before_reuse(self):
        a=snapshot([(0,True)])
        b=snapshot([(0,True),(0,False),(1,True)])
        self.assertEqual(transition(a,b,48),'')
    def test_empty_pool_has_no_forbidden_admission(self):
        s=baseline(); s.update(halted=1,valid=0,faults=1,rejected=1,attempts=1,levels=1)
        with self.assertRaises(ValueError): validate(s)
    def test_occupied_and_orphan_fault_context(self):
        for s,fault in [(baseline(),4),(snapshot([(0,True)]),2)]:
            s.update(halted=1,valid=0,faults=fault,rejected=1,attempts=s['attempts']+1)
            with self.assertRaises(ValueError): validate(s)
    def test_invalid_counts(self):
        for k,v in [('active',1),('use0',1),('depth',1),('peak',5),('rounds',1),('faults',3),('now',True),('stamp0',1)]:
            with self.subTest(k=k):
                s=baseline(); s[k]=v
                with self.assertRaises(ValueError): validate(s)
    def test_parser_recovery_crc_tail_and_foreign_protocol(self):
        m=Model('synthetic malformed','replay')
        m.feed(b'x'*900+b'DF1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors,0); self.assertEqual(m.latest,baseline())
        n=m.errors; m.feed(b'DF1,S,1*0000\n'); self.assertGreater(m.errors,n)
        m.feed(b'DF1,S'); n=m.errors; m.finish(); self.assertEqual(m.errors,n+1)
        m=Model('foreign','replay'); m.feed(stream(baseline()).replace(b'DF1,',b'EF1,'))
        self.assertFalse(m.policy); self.assertIsNone(m.latest)
    def test_policy_reset_and_halt_freeze(self):
        m=Model('bad policy','replay'); m.feed(frame('P',[3,1,5,4,2,1])); self.assertTrue(m.frozen)
        m=Model('reset','replay'); m.feed(stream(baseline())+frame('B',[42])); self.assertTrue(m.frozen)
        a=snapshot([(0,True)])
        a.update(halted=1,valid=0,faults=1,rejected=1,attempts=2,levels=1)
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
            os.write(master,stream(snapshot([(0,True)])))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertIn('Eligible allocation tags: [3]',app.summary.get('1.0','end'))
            self.assertIn('Distance-fit policy',app.summary.get('1.0','end'))
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
                self.assertIn('closer admission despite farther vacancy',app.summary.get('1.0','end'))
                app.path.set('/dev/null'); app.connect(); self.assertTrue(app.message)
                self.assertEqual(app.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                saved=json.loads(out.read_text()); self.assertEqual(saved['latest']['active'],1)
                self.assertEqual(saved['author'],'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): app.export()
                self.assertNotIn('Saved private',app.message)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
