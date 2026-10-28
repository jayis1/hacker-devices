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
import console as a
FIXTURE=subprocess.check_output([str(ROOT/'firmware/build/fixture')])
class Tests(unittest.TestCase):
    def model(self):
        m=a.Model('synthetic C encoder','replay'); m.feed(FIXTURE); return m

    def send(self,m,s):
        m.feed(a.frame('S',[s[k] for k in a.FIELDS]))

    def test_fixture(self):
        m=self.model(); m.finish()
        self.assertEqual(m.errors,0); self.assertFalse(m.frozen)
        self.assertEqual(m.latest['entries0'],1)
        self.assertEqual(m.latest['duration0'],199)
        self.assertEqual(m.latest['duration1'],99)
        self.assertEqual(m.latest['duration3'],99)
        self.assertFalse(m.live())

    def test_invalid_snapshots(self):
        for changes in [dict(levels=16),dict(valid=16),dict(halted=1),
                        dict(active0=1),dict(duration0=1),dict(age0=1),
                        dict(entries0=1),dict(faults=1),dict(gaps=1),
                        dict(entries0=1,faults=1,duration0=3,maximum0=2),
                        dict(entries0=2,faults=1,duration0=5,maximum0=2)]:
            s=dict.fromkeys(a.FIELDS,0); s.update(changes)
            m=a.Model('synthetic','replay'); m.feed(a.frame('P',[3,1,4,6]))
            self.send(m,s)
            self.assertEqual(m.errors,1,changes); self.assertIsNone(m.latest)

    def test_first_status_not_live(self):
        m=a.Model('synthetic','live'); m.connected=True
        lines=FIXTURE.splitlines(keepends=True)
        m.feed(b''.join(lines[:3])); self.assertFalse(m.live())
        m.feed(lines[3]); self.assertTrue(m.live())
        m.connected=False; self.assertFalse(m.live())

    def test_rate_bound(self):
        m=self.model(); s=m.latest.copy(); s['now']+=1
        s['duration0']+=2; s['maximum0']+=2
        self.send(m,s); self.assertIn('Duration exceeds',m.frozen)

    def test_bounds_crc_tail(self):
        m=a.Model('synthetic','replay'); m.feed(b'x'*2000+b'\n'+FIXTURE)
        self.assertGreater(m.errors,0); self.assertIsNotNone(m.latest)
        m.feed(b'EL1,S,123'); m.finish(); self.assertGreater(m.errors,1)
        m=a.Model('synthetic','replay'); m.feed(FIXTURE.replace(b'EL1,S',b'EL1,X'))
        self.assertGreater(m.errors,0)
        with self.assertRaises(ValueError): m.finish()

    def test_resync(self):
        m=a.Model('synthetic','replay'); m.feed(b'EL1,S,42'+FIXTURE)
        self.assertEqual(m.errors,1); self.assertEqual(m.latest['entries0'],1)

    def test_reset_and_policy(self):
        m=self.model(); m.feed(a.frame('B',[0])); self.assertTrue(m.frozen)
        m.feed(a.frame('P',[3,1,4,6])); self.assertTrue(m.frozen)
        m=a.Model('synthetic','replay'); m.feed(a.frame('P',[1])); m.feed(FIXTURE)
        self.assertIsNone(m.latest)

    def test_duplicates_and_rollback(self):
        m=self.model(); at=m.received
        m.feed(FIXTURE.splitlines(keepends=True)[-1]); self.assertEqual(m.received,at)
        s=m.latest.copy(); s['tx_fail']+=1
        self.send(m,s); self.assertTrue(m.frozen)
        m=self.model(); s=m.latest.copy(); s['now']-=1
        self.send(m,s); self.assertTrue(m.frozen)
        m=self.model(); s=m.latest.copy(); s['now']+=100; s['observed']-=1
        self.send(m,s); self.assertTrue(m.frozen)

    def test_freshness_and_backlog(self):
        m=a.Model('synthetic','live'); m.connected=True
        with patch.object(a.time,'monotonic',return_value=10): m.feed(FIXTURE)
        s=m.latest.copy(); s['now']+=100
        with patch.object(a.time,'monotonic',return_value=12):
            self.assertFalse(m.live()); self.send(m,s)
        self.assertIn('pause',m.frozen)
        m=a.Model('synthetic','live'); m.connected=True
        with patch.object(a.time,'monotonic',return_value=10): m.feed(FIXTURE)
        for i in range(1,10):
            s['now']+=1
            with patch.object(a.time,'monotonic',return_value=10+i*.2): self.send(m,s)
        self.assertIn('diverged',m.frozen)

    def test_files(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'out.json'; m=self.model(); m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): a.read_replay(fifo)
            big=Path(d)/'big'; big.write_bytes(b'x'*(a.FILE_LIMIT+1))
            with self.assertRaises(ValueError): a.read_replay(big)
            p=Path(d)/'empty.json'; m=a.Model('synthetic','replay'); m.errors=2; m.export(p)
            self.assertEqual(json.loads(p.read_text())['errors'],2)

    def test_ui_serial(self):
        master,slave=pty.openpty(); root=tk.Tk(); ui=a.Console(root)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            old=ui.port; ui.connect(); self.assertIs(ui.port,old)
            os.write(master,FIXTURE)
            for _ in range(15): root.update(); time.sleep(.02)
            self.assertTrue(ui.model.live())
            self.assertEqual(ui.model.latest['entries0'],1)
            self.assertIn('duration0: 199',ui.summary.get('1.0','end'))
            self.assertIn('Overlap 0/1',ui.summary.get('1.0','end'))
            self.assertEqual(len(ui.model.history),5)
            for tab in ui.tabs.tabs(): ui.tabs.select(tab); root.update()
            os.close(master); master=-1
            for _ in range(15): root.update(); time.sleep(.02)
            self.assertFalse(ui.model.connected); self.assertEqual(ui.model.origin,'live')
        finally:
            ui.close(); root.destroy(); os.close(slave)
            if master>=0: os.close(master)

    def test_ui_replay_export(self):
        root=tk.Tk(); ui=a.Console(root)
        try:
            with tempfile.TemporaryDirectory() as d:
                p=Path(d)/'synthetic.txt'; p.write_bytes(FIXTURE)
                ui.load(p); root.update(); self.assertEqual(ui.model.origin,'replay')
                out=Path(d)/'report.json'
                with patch.object(a.filedialog,'asksaveasfilename',return_value=str(out)):
                    ui.export(); self.assertEqual(json.loads(out.read_text())['latest']['duration0'],199)
                    ui.export(); self.assertIn('File exists',ui.message)
                retained=ui.model; ui.path.set(str(Path(d)/'missing')); ui.connect()
                self.assertIs(ui.model,retained); self.assertIsNone(ui.port)
                empty=Path(d)/'empty'; empty.write_bytes(b'')
                with self.assertRaises(ValueError): ui.load(empty)
                self.assertIs(ui.model,retained)
        finally:
            ui.close(); root.destroy()
if __name__=='__main__': unittest.main()
