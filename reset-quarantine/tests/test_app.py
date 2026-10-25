# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import os
from pathlib import Path
import pty
import tempfile
import termios
import unittest
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('audit',Path(__file__).resolve().parents[1]/'app/audit.py')
assert spec is not None and spec.loader is not None
audit=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def frame(body):
    body=body.encode()
    return body+f'*{audit.crc(body):04X}\n'.encode()

S='RQ1,S,100,0,1,1,90,0,1,0,0,0,1,0,0,0,0'
class Tests(unittest.TestCase):
    def test_policy_and_freshness(self):
        m=audit.Model('test'); m.connected=True
        m.feed(frame(S)); self.assertFalse(m.fresh())
        m.feed(frame('RQ1,P,250,3')); self.assertTrue(m.fresh())
        m.received-=2; self.assertFalse(m.fresh())
        m.replay=True; m.received=audit.time.monotonic(); self.assertFalse(m.fresh())

    def test_restart_counters_and_policy(self):
        m=audit.Model('test'); m.feed(frame('RQ1,P,250,3')+frame(S))
        m.feed(frame(S.replace(',100,',',101,').replace(',90,0,1,',',90,0,0,')))
        self.assertTrue(m.restart)
        m.feed(frame('RQ1,P,250,3')); self.assertTrue(m.restart)
        m.feed(frame('RQ1,B,0,250,3')); self.assertFalse(m.restart)
        self.assertIsNone(m.status)

    def test_fragment_recovery_and_bounds(self):
        m=audit.Model('test')
        m.feed(b'RQ1,S,12'+frame(S)); self.assertEqual(m.errors,1)
        self.assertEqual(m.status[0],100)
        m.feed(b'x'*5000); self.assertLessEqual(len(m.buffer),384)
        m.feed(b'\n'+frame('RQ1,S,0'))
        m.feed(frame(S.replace(',100,',',4294967296,')))
        m.feed(frame(S.replace(',100,',',-1,')))
        m.feed(frame(S.replace(',0,1,1,',',16,1,1,')))
        self.assertGreater(m.errors,5)
        for byte in frame('RQ1,P,250,3'): m.feed(bytes([byte]))
        self.assertTrue(m.policy_known)

    def test_incompatible_freezes_status(self):
        m=audit.Model('test'); m.feed(frame(S))
        m.feed(frame('RQ1,P,251,3'))
        m.feed(frame(S.replace(',100,',',101,')))
        self.assertEqual(m.status[0],100)
        self.assertTrue(m.incompatible)

    def test_events_and_snapshot(self):
        m=audit.Model('fixture',replay=True)
        for i in range(300): m.feed(frame(f'RQ1,E,{i},12,7,1'))
        self.assertEqual(len(m.events),256)
        m.feed(frame('RQ1,E,300,12,9,1')); self.assertEqual(m.errors,1)
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'evidence.json'; m.export(p)
            self.assertEqual(p.stat().st_mode&0o777,0o600)
            self.assertIn('jayis1',p.read_text())
            with self.assertRaises(FileExistsError): m.export(p)
        self.assertTrue(m.snapshot()['replay'])
        for page in (1,2,3): self.assertTrue(audit.screen_lines(m,page))

    def test_serial_restore_and_failure(self):
        master,slave=pty.openpty()
        original=termios.tcgetattr(slave)
        try:
            port=audit.Port(os.ttyname(slave))
            os.write(master,b'hello\n')
            self.assertEqual(port.poll(),b'hello\n')
            port.close()
            self.assertEqual(termios.tcgetattr(slave),original)
            with patch.object(audit.termios,'tcflush',side_effect=termios.error(5,'test')):
                with self.assertRaises(termios.error): audit.Port(os.ttyname(slave))
            self.assertEqual(termios.tcgetattr(slave),original)
            port=audit.Port(os.ttyname(slave))
            os.close(master); master=None
            port.close() # hangup must not make restoration fatal
        finally:
            if master is not None: os.close(master)
            os.close(slave)

if __name__=='__main__': unittest.main()
