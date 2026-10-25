# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
import json
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT/'app/audit.py')
assert spec is not None and spec.loader is not None
a = importlib.util.module_from_spec(spec)
spec.loader.exec_module(a)

def frame(kind, values):
    body = ('RW1,' + kind + ',' + ','.join(map(str,values))).encode()
    return body + f'*{binascii.crc_hqx(body,65535):04X}\n'.encode()

def status(t=100, **changes):
    d = dict(zip(a.FIELDS,[t,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0]))
    d.update(changes)
    return frame('S',[d[k] for k in a.FIELDS])

class ModelTests(unittest.TestCase):
    def setUp(self):
        self.m = a.Model('synthetic')
        self.m.feed(frame('P',a.POLICY))
    def test_real_firmware(self):
        data = subprocess.check_output([str(ROOT/'firmware/build/fixture')])
        m = a.Model('synthetic')
        for byte in data:
            m.feed(bytes([byte]))
        self.assertFalse(m.frozen)
        self.assertEqual(m.errors,0)
        self.assertEqual(m.latest['receipts'],1)
        self.assertEqual(m.latest['abandoned'],1)
        self.assertTrue(m.latest['faults'] & 8)
        self.assertEqual(len(m.history),9)
    def test_duplicate_does_not_refresh(self):
        self.m.feed(status())
        age = self.m.updated
        self.m.feed(status())
        self.assertEqual(age,self.m.updated)
        self.m.feed(status(levels=8))
        self.assertTrue(self.m.frozen)
    def test_counter_and_fault_regression(self):
        self.m.feed(status(faults=1))
        self.m.feed(status(200))
        self.assertTrue(self.m.frozen)
    def test_bad_invariants(self):
        for bad in [status(count=9),status(waiting=1),status(receipts=1),status(max_append=51),status(ready=0,count=1)]:
            self.m.feed(bad)
        self.assertEqual(self.m.errors,5)
        self.assertIsNone(self.m.latest)
    def test_append_conservation(self):
        self.m.feed(status(actions=1,count=1,waiting=0,appended=0))
        self.m.feed(status(actions=1,count=1,waiting=1,appended=1))
        self.assertEqual(self.m.errors,2)
    def test_counter_regression(self):
        self.m.feed(status(actions=1,abandoned=1))
        self.m.feed(status(200))
        self.assertTrue(self.m.frozen)
    def test_crc_and_resynchronization(self):
        self.m.feed(b'junkRW1,S,1,2')
        self.m.feed(status())
        self.assertIsNotNone(self.m.latest)
        self.m.feed(b'RW1,S,0*0000\n')
        self.assertGreater(self.m.errors,0)
        self.m.feed(b'x'*10000)
        self.assertLessEqual(len(self.m.buffer),384)
    def test_policy_and_boot(self):
        self.m.feed(status())
        self.m.feed(frame('B',[0]))
        self.assertTrue(self.m.frozen)
        self.m.feed(frame('P',a.POLICY))
        self.assertTrue(self.m.frozen)
    def test_wrap(self):
        self.m.feed(status(4294967290))
        self.m.feed(status(20))
        self.assertFalse(self.m.frozen)
    def test_buffered_hangup(self):
        class Poller:
            def poll(self, timeout):
                return [(123,a.select.POLLIN | a.select.POLLHUP)]
        serial = object.__new__(a.Serial)
        serial.fd = 123
        serial.poller = Poller()
        with patch.object(a.os,'read',side_effect=[status(),b'']):
            self.assertEqual(serial.read(),status())
            with self.assertRaises(OSError): serial.read()
    def test_constructor_restores_terminal(self):
        master, slave = pty.openpty()
        before = a.termios.tcgetattr(slave)
        try:
            with patch.object(a.termios,'tcflush',side_effect=OSError('injected')):
                with self.assertRaises(OSError): a.Serial(os.ttyname(slave))
            self.assertEqual(a.termios.tcgetattr(slave),before)
        finally:
            os.close(master); os.close(slave)
    def test_export_and_input_bounds(self):
        self.m.feed(status())
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/'report.json'
            self.m.export(p)
            report = json.loads(p.read_text())
            self.assertEqual(report['author'],'jayis1')
            self.assertEqual(report['provenance'],'synthetic')
            self.assertEqual(p.stat().st_mode & 0o777,0o600)
            with self.assertRaises(FileExistsError): self.m.export(p)
            fifo = Path(d)/'fifo'; os.mkfifo(fifo)
            with self.assertRaises(ValueError): a.read_capture(fifo)
            huge = Path(d)/'huge'; huge.write_bytes(b'x'*(a.MAX_FILE+1))
            with self.assertRaises(ValueError): a.read_capture(huge)
    def test_pty_and_gui(self):
        import tkinter as tk
        root = tk.Tk(); root.withdraw()
        ui = a.App(root)
        master,slave = pty.openpty()
        try:
            ui.path.set(os.ttyname(slave)); ui.connect()
            data = subprocess.check_output([str(ROOT/'firmware/build/fixture')])
            os.write(master,data)
            end = time.monotonic()+0.25
            while time.monotonic()<end:
                root.update(); time.sleep(0.01)
            self.assertEqual(ui.model.latest['receipts'],1)
            self.assertIn('receipts: 1',ui.summary.get())
            self.assertEqual(len(ui.rows.get_children()),9)
            os.close(master); master = None
            end = time.monotonic()+0.15
            while time.monotonic()<end:
                root.update(); time.sleep(0.01)
            self.assertIsNone(ui.serial)
            self.assertEqual(ui.model.provenance,'live')
        finally:
            if master is not None: os.close(master)
            os.close(slave); ui.close()

if __name__ == '__main__': unittest.main()
