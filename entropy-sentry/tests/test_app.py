# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
import os
from pathlib import Path
import pty
import subprocess
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('audit', ROOT/'app/audit.py')
assert spec is not None and spec.loader is not None
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
def frame(body):
    b=body.encode('ascii')
    return b+f'*{audit.crc(b):04X}\n'.encode()
class Tests(unittest.TestCase):
    def test_duplicate_does_not_refresh(self):
        m=audit.Model('test')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture']))
        m.received-=2
        old=m.received
        values=[m.status[k] for k in audit.FIELDS]
        m.feed(frame('ES1,S,'+','.join(map(str,values))))
        self.assertEqual(m.received,old)
    def test_sticky_mask_cannot_disappear(self):
        m=audit.Model('test')
        m.feed(subprocess.check_output([ROOT/'firmware/build/fixture']))
        values=[m.status[k] for k in audit.FIELDS]
        values[0]+=100
        values[2]=1
        values[3]=0
        m.feed(frame('ES1,S,'+','.join(map(str,values))))
        self.assertTrue(m.restart)
        self.assertEqual(m.status['faults'],3)
    def test_real_encoder(self):
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m=audit.Model('synthetic',True)
        for byte in data:
            m.feed(bytes([byte]))
        self.assertEqual(m.errors,0)
        self.assertEqual(m.status['samples'],128)
        self.assertEqual(m.status['faults'],3)
        self.assertFalse(m.fresh())
        self.assertGreater(m.status['unsafe_ms'],0)
        for page in (1,2,3):
            self.assertGreater(len(audit.screen_lines(m,page)),4)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'e.json'
            m.export(path)
            self.assertEqual(path.stat().st_mode&0o777,0o600)
            with self.assertRaises(FileExistsError): m.export(path)
    def test_resync_bounds(self):
        m=audit.Model('test')
        m.feed(b'ES1,S,partial'+frame('ES1,P,32,64,49,10,100,2'))
        self.assertTrue(m.policy_known)
        m.feed(b'x'*2000+b'\n')
        self.assertLessEqual(len(m.buffer),384)
        m.feed(frame('ES1,P,32,64,49,10,100,3'))
        self.assertTrue(m.incompatible)
        m.feed(frame('ES1,B,-1'))
        m.feed(frame('ES1,B,4294967296'))
        self.assertGreater(m.errors,3)
    def test_restart_and_freshness(self):
        m=audit.Model('test')
        data=subprocess.check_output([ROOT/'firmware/build/fixture'])
        m.feed(data)
        m.connected=True
        self.assertTrue(m.fresh())
        last=m.status.copy()
        fields=[last[k] for k in audit.FIELDS]
        fields[0]-=1
        m.feed(frame('ES1,S,'+','.join(map(str,fields))))
        self.assertTrue(m.restart)
        m.feed(frame('ES1,P,32,64,49,10,100,2'))
        self.assertTrue(m.restart)
        self.assertFalse(m.fresh())
        m.connected=False
        self.assertEqual(m.snapshot()['status'],last)
    def test_port_and_disconnect(self):
        master,slave=pty.openpty()
        port=audit.Port(os.ttyname(slave))
        try:
            os.write(master,b'hello\n')
            import time
            time.sleep(.02)
            self.assertEqual(port.poll(),b'hello\n')
            os.close(master)
            master=None
            self.assertEqual(port.poll(),b'')
        finally:
            port.close()
            if master is not None: os.close(master)
            os.close(slave)
if __name__=='__main__': unittest.main()
