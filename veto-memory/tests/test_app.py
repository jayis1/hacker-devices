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
from console import Console, Model, Serial, frame, read_replay
from contract import FIELDS, validate

class Evidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        cls.lines=cls.data.splitlines(keepends=True)
    def model(self):
        m=Model('synthetic C fixture','replay'); m.feed(self.data); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        return m
    def test_production_encoder(self):
        m=self.model(); s=m.latest
        for key,value in dict(accepted=1,vetoed=2,early=1,repeated=1,cancelled=1,abandoned=1,halted=1).items():
            self.assertEqual(s[key],value)
        self.assertFalse(m.live())
    def test_impossible(self):
        good=self.model().latest
        for field in ('rounds','accepted','commits','gaps','faults','phase','veto'):
            bad=good.copy(); bad[field]+=1
            with self.assertRaises(ValueError): validate(bad)
        m=Model('bad','replay'); m.feed(self.lines[1]); bad=good.copy(); bad['rounds']+=1
        m.feed(frame('S',[bad[k] for k in FIELDS])); self.assertEqual(m.errors,1)
    def test_crc_and_resync(self):
        m=Model('truncated','replay')
        m.feed(b'VM1,S,broken'+self.data); m.finish()
        self.assertEqual(m.errors,1); self.assertIsNotNone(m.latest)
        m.feed(b'VM1,S,1*0000\n'); self.assertEqual(m.errors,2)
        m.feed(b'VM1,S,'); m.finish(); self.assertEqual(m.errors,3)
        m.feed(b'x'*10000); self.assertLessEqual(len(m.buffer),768)
    def test_duplicate_rollback_and_policy(self):
        m=Model('fixture','replay'); m.feed(b''.join(self.lines[:4]))
        before=m.received; m.feed(self.lines[3]); self.assertEqual(m.received,before)
        bad=m.latest.copy(); bad['tx_fail']+=1
        m.feed(frame('S',[bad[k] for k in FIELDS])); self.assertIn('duplicate',m.frozen)
        m=Model('fixture','replay'); m.feed(b''.join(self.lines[:4])); m.feed(self.lines[2])
        self.assertIn('rollback',m.frozen)
        m=self.model(); m.feed(self.lines[0]); self.assertIn('Boot',m.frozen)
        m=Model('unknown','replay'); m.feed(frame('P',[3,1,2,99])); self.assertIn('Unsupported',m.frozen)
    def test_veto_cannot_vanish_within_round(self):
        m=Model('fixture','replay'); m.feed(b''.join(self.lines[:9]))
        self.assertEqual(m.latest['veto'],1)
        bad=m.latest.copy(); bad['now']+=1; bad['observed']+=1; bad['veto']=0
        m.feed(frame('S',[bad[k] for k in FIELDS])); self.assertIn('Veto',m.frozen)
    def test_live_progress_and_backlog(self):
        m=Model('fake transport','live'); m.connected=True
        with patch('console.time.monotonic',return_value=10): m.feed(b''.join(self.lines[:3]))
        self.assertFalse(m.progress)
        with patch('console.time.monotonic',return_value=10.1):
            m.feed(self.lines[3]); self.assertTrue(m.live())
        with patch('console.time.monotonic',return_value=12):
            self.assertFalse(m.live()); m.feed(self.lines[4])
        self.assertIn('Reception pause',m.frozen)
    def test_replay_export(self):
        m=self.model()
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'evidence.json'; m.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            data=json.loads(path.read_text()); self.assertEqual(data['author'],'jayis1')
            self.assertEqual(data['origin'],'replay')
            with self.assertRaises(FileExistsError): m.export(path)
            fifo=Path(temp)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): read_replay(fifo)
            huge=Path(temp)/'huge'; huge.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(huge)
            with self.assertRaises(ValueError): Model('empty','replay').finish()
    def test_gui_serial_and_screens(self):
        root=tk.Tk(); ui=Console(root); master,slave=pty.openpty()
        try:
            ui.path.set(os.ttyname(slave)); ui.connect(); self.assertTrue(ui.model.connected)
            port=ui.port; ui.connect(); self.assertIs(ui.port,port)
            os.write(master,b''.join(self.lines[:3])); root.update(); time.sleep(.03); root.update()
            time.sleep(.1); os.write(master,self.lines[3]); time.sleep(.03); root.update()
            self.assertTrue(ui.model.live()); self.assertEqual(ui.model.errors,0)
            for index in range(3): ui.tabs.select(index); root.update()
            self.assertIn('rounds',ui.summary.get('1.0','end'))
            self.assertIn('phase',ui.history.get('1.0','end'))
            self.assertIn('live',ui.integrity.get('1.0','end'))
            ui.model.feed(b'VM1,S,partial')
            os.close(master); master=None; time.sleep(.04); root.update()
            self.assertFalse(ui.model.connected); self.assertIsNone(ui.port)
            self.assertEqual(ui.model.errors,1)
            self.assertFalse(ui.model.buffer)
            self.assertIsNotNone(ui.model.latest)
            with tempfile.TemporaryDirectory() as temp:
                path=Path(temp)/'synthetic.vm1'; path.write_bytes(self.data)
                ui.load(path); self.assertEqual(ui.model.origin,'replay')
                out=Path(temp)/'report.json'
                with patch('console.filedialog.asksaveasfilename',return_value=str(out)): ui.export()
                self.assertEqual(json.loads(out.read_text())['latest']['vetoed'],2)
                ui.path.set('/nonexistent/veto-memory'); ui.connect()
                self.assertEqual(ui.model.origin,'replay'); self.assertTrue(ui.message)
        finally:
            ui.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)

if __name__=='__main__': unittest.main()
