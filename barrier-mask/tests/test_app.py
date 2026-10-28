# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import json
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0,str(ROOT/'app'))
spec=importlib.util.spec_from_file_location('console',ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
a=importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)
FIXTURE=subprocess.check_output([str(ROOT/'firmware/build/fixture')])
class Tests(unittest.TestCase):
    def test_first_status_is_not_progress(self):
        m=a.Model('synthetic','live'); m.connected=True
        lines=FIXTURE.splitlines(keepends=True)
        m.feed(lines[0]+lines[1])
        self.assertFalse(m.live())
        m.feed(lines[2]); self.assertFalse(m.live())
        m.feed(lines[3]); self.assertTrue(m.live())

    def test_impossible_barriers(self):
        for changes in [dict(mask=16),dict(active=1),dict(completed=1),dict(duplicate=1),
                        dict(ready=1,faults=32,gaps=1,missed=1),dict(age=1002),
                        dict(accepted=1),dict(rounds=1,abandoned=1),
                        dict(timeouts=1,faults=4),dict(releases=0,early=1,faults=2)]:
            s=dict.fromkeys(a.FIELDS,0); s.update(changes)
            m=a.Model('synthetic','replay'); m.feed(a.frame('P',[4,5,3,20,1000]))
            m.feed(a.frame('S',[s[k] for k in a.FIELDS]))
            self.assertEqual(m.errors,1,changes)
            self.assertIsNone(m.latest)

    def test_fixture(self):
        m=a.Model('synthetic','replay'); m.feed(FIXTURE); m.finish()
        self.assertEqual(m.latest['completed'],1)
        self.assertEqual(m.latest['duplicate'],1)
        self.assertEqual(m.errors,0)
        self.assertFalse(m.live())
    def test_bounds_and_crc(self):
        m=a.Model('test','replay'); m.feed(b'x'*2000+b'\n'+FIXTURE)
        self.assertGreater(m.errors,0)
        self.assertEqual(m.latest['duplicate'],1)
        m.feed(FIXTURE[-10:]); m.finish(); self.assertGreater(m.errors,1)
        m=a.Model('test','replay'); m.feed(FIXTURE.replace(b'BM1,S',b'BM1,X'))
        self.assertGreater(m.errors,0)
    def test_impossible(self):
        m=a.Model('test','replay'); m.feed(FIXTURE)
        values=[m.latest[k] for k in a.FIELDS]; values[13]+=1
        m.feed(a.frame('S',values)); self.assertGreater(m.errors,0)
    def test_restart_duplicate(self):
        m=a.Model('test','replay'); m.feed(FIXTURE)
        at=m.received; m.feed(FIXTURE.splitlines(keepends=True)[-1])
        self.assertEqual(m.received,at)
        values=[m.latest[k] for k in a.FIELDS]; values[0]-=1
        m.feed(a.frame('S',values)); self.assertTrue(m.frozen)
    def test_file_boundaries(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'evidence.json'; m=a.Model('test','replay'); m.feed(FIXTURE)
            m.export(p); self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertEqual(json.loads(p.read_text())['author'],'jayis1')
            with self.assertRaises(FileExistsError): m.export(p)
            fifo=Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): a.read_replay(fifo)
            large=Path(d)/'large'; large.write_bytes(b'x'*(a.FILE_LIMIT+1))
            with self.assertRaises(ValueError): a.read_replay(large)
    def test_pause_and_backlog(self):
        m=a.Model('synthetic','live'); m.connected=True
        with patch.object(a.time,'monotonic',return_value=10.0): m.feed(FIXTURE)
        values=[m.latest[k] for k in a.FIELDS]; values[0]+=100
        with patch.object(a.time,'monotonic',return_value=12.0):
            m.feed(a.frame('S',values)); self.assertFalse(m.live())
        self.assertIn('pause',m.frozen)
        m=a.Model('synthetic','live'); m.connected=True
        with patch.object(a.time,'monotonic',return_value=10.0): m.feed(FIXTURE)
        for i in range(1,10):
            values[0]+=1
            with patch.object(a.time,'monotonic',return_value=10+i*.2):
                m.feed(a.frame('S',values))
        self.assertIn('diverged',m.frozen)
    def test_fragment_reset_and_policy(self):
        m=a.Model('synthetic','replay'); m.feed(b'BM1,S,42'+FIXTURE)
        self.assertEqual(m.errors,1); self.assertIsNotNone(m.latest)
        m.feed(a.frame('B',[0])); self.assertTrue(m.frozen)
        m.feed(a.frame('P',[4,5,3,20,1000])); self.assertTrue(m.frozen)
        m=a.Model('synthetic','replay'); m.feed(a.frame('P',[9,5,3,20]))
        m.feed(FIXTURE); self.assertIsNone(m.latest)
    def test_export_empty_metadata(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'empty.json'; m=a.Model('synthetic','replay')
            m.errors=2; m.export(p)
            report=json.loads(p.read_text())
            self.assertEqual(report['errors'],2)
            self.assertEqual(report['history'],[])
    def test_ui_serial(self):
        master,slave=pty.openpty(); root=tk.Tk(); ui=a.Console(root)
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            os.write(master,FIXTURE)
            for _ in range(15): root.update(); time.sleep(.02)
            self.assertEqual(ui.model.latest['duplicate'],1)
            self.assertIn('completed: 1',ui.summary.get('1.0','end'))
            self.assertTrue(ui.model.live())
            for tab in ui.tabs.tabs(): ui.tabs.select(tab); root.update()
            os.close(master); master=-1
            for _ in range(15): root.update(); time.sleep(.02)
            self.assertFalse(ui.model.connected)
            self.assertEqual(ui.model.origin,'live')
        finally:
            ui.close(); root.destroy(); os.close(slave)
            if master>=0: os.close(master)
    def test_ui_replay_export_and_failed_connection(self):
        root=tk.Tk(); ui=a.Console(root)
        try:
            with tempfile.TemporaryDirectory() as directory:
                capture=Path(directory)/'synthetic.txt'; capture.write_bytes(FIXTURE)
                report=Path(directory)/'report.json'
                ui.load(capture); root.update()
                self.assertEqual(ui.model.origin,'replay')
                self.assertIn('completed: 1',ui.summary.get('1.0','end'))
                with patch.object(a.filedialog,'asksaveasfilename',return_value=str(report)):
                    ui.export()
                    self.assertEqual(json.loads(report.read_text())['latest']['duplicate'],1)
                    ui.export()
                    self.assertIn('File exists',ui.message)
                retained=ui.model
                ui.path.set(str(Path(directory)/'missing-port')); ui.connect()
                self.assertIs(ui.model,retained)
                self.assertIsNone(ui.port)
                ui.disconnect()
                self.assertEqual(ui.model.origin,'replay')
        finally:
            ui.close(); root.destroy()
if __name__=='__main__': unittest.main()
