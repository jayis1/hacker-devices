# Author: jayis1. Synthetic application and actual Tk/PTY integration tests.
import importlib.util
from pathlib import Path
import subprocess
import unittest
import tempfile
import os
import binascii
import time
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('console',ROOT/'app/console.py')
assert spec is not None and spec.loader is not None
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
def frame(body):
    return body+b'*'+f'{binascii.crc_hqx(body,65535):04X}'.encode()+b'\n'
class Tests(unittest.TestCase):
    def stream(self):
        return subprocess.check_output([ROOT/'firmware/build/fixture'])
    def model(self):
        m=a.Model('synthetic',False);m.feed(self.stream(),0);m.eof();return m
    def test_fixture(self):
        m=self.model();self.assertEqual(m.errors,0)
        self.assertEqual(m.status['c0_completed'],1)
        self.assertEqual(m.status['c1_shorts'],1)
        self.assertEqual(m.status['c2_longs'],1)
        self.assertEqual(m.status['c3_rapid'],1)
        self.assertFalse(m.fresh(0))
    def test_impossible(self):
        m=self.model();v=[m.status[k] for k in a.FIELDS]
        for key,value in [('c0_starts',99),('c0_peak',202),('c0_shorts',2),('faults',0)]:
            bad=v.copy();bad[a.FIELDS.index(key)]=value
            with self.assertRaises(ValueError):a.validate(bad)
    def test_framing(self):
        m=a.Model('synthetic',False);m.feed(b'PE1,S,123'+self.stream(),0);m.eof()
        self.assertEqual(m.errors,1);self.assertIsNotNone(m.status)
        m.feed(b'PE1,S',1);m.eof();self.assertEqual(m.errors,2)
        m.feed(b'x'*2000,1);self.assertLess(len(m.buffer),768)
    def test_duplicate_and_boot(self):
        m=self.model();s=m.status.copy();s['now']=999
        m.feed(frame(('PE1,S,'+','.join(str(s[k]) for k in a.FIELDS)).encode()),1)
        self.assertTrue(m.frozen)
        m=self.model();m.feed(frame(b'PE1,B,0'),2);self.assertTrue(m.frozen)
    def test_files(self):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'report.json';m=self.model();m.export(p,1)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError):m.export(p,2)
            fifo=Path(d)/'pipe';os.mkfifo(fifo)
            with self.assertRaises(ValueError):a.read_replay(fifo)
    def test_live_pause(self):
        rows=self.stream().splitlines(keepends=True)
        m=a.Model('PTY',True);m.connected=True
        for row in rows[:4]:m.feed(row,0)
        self.assertTrue(m.fresh(0))
        m.feed(rows[4],2);self.assertTrue(m.frozen)
    def test_gui(self):
        import tkinter as tk
        root=tk.Tk();w=a.Window(root)
        master,slave=os.openpty();w.port.set(os.ttyname(slave));w.connect()
        def pump():
            for _ in range(8):root.update();time.sleep(.02)
        # Feed policy plus one actual encoder snapshot, avoiding fake time acceleration.
        rows=self.stream().splitlines(keepends=True)
        os.write(master,b''.join(rows[:3]));pump()
        self.assertIsNotNone(w.model.status)
        self.assertIn('LIVE',w.views['Envelope'].get('1.0','end'))
        for name in w.views:
            self.assertTrue(w.views[name].get('1.0','end').strip())
        os.close(master);pump();self.assertFalse(w.model.connected)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'fixture.txt';p.write_bytes(self.stream());w.replay(p);pump()
            self.assertEqual(w.model.status['c3_rapid'],1)
            self.assertFalse(w.model.live_source)
            w.model.export(Path(d)/'e.json',time.monotonic())
        os.close(slave);w.close()
if __name__=='__main__':unittest.main()
