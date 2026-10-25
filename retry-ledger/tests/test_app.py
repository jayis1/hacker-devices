# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
from pathlib import Path
import tempfile
import unittest
import subprocess
import os
import pty
import termios
import time
SPEC = importlib.util.spec_from_file_location('audit', Path(__file__).resolve().parents[1]/'app/audit.py')
assert SPEC is not None and SPEC.loader is not None
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)

def frame(body):
    raw = body.encode('ascii')
    return raw + f'*{binascii.crc_hqx(raw,65535):04X}\n'.encode()

class AppTests(unittest.TestCase):
    def fixture(self):
        return subprocess.check_output([str(Path(__file__).resolve().parents[1]/'firmware/build/fixture')])

    def test_actual_encoder(self):
        m = audit.Model('replay')
        m.feed(self.fixture())
        self.assertTrue(m.compatible)
        self.assertEqual(m.latest['failures'],3)
        self.assertEqual(m.latest['cycles'],1)
        self.assertEqual(m.latest['strikes'],0)
        self.assertEqual(m.latest['access_ms'],100)
        self.assertTrue(m.latest['faults'] & 2)
        self.assertEqual(m.errors,0)
        self.assertGreater(len(m.events),5)

    def test_resync_bounds_corruption(self):
        m=audit.Model('replay')
        m.feed(b'RL1,S,12'+frame('RL1,P,3,5000,20,3,20'))
        self.assertTrue(m.compatible)
        m.feed(b'x'*10000)
        self.assertLess(len(m.buffer),384)
        m.feed(b'\nRL1,P,3,5000,20,3,20*0000\n')
        self.assertGreater(m.errors,0)

    def test_restart_and_duplicate(self):
        m=audit.Model('live')
        m.feed(self.fixture())
        stamp=m.updated
        body='RL1,S,'+','.join(str(m.latest[k]) for k in audit.FIELDS)
        m.feed(frame(body))
        self.assertEqual(m.updated,stamp)
        m.feed(frame(body.replace('5500,','5501,').replace(',3,0,1,',',2,0,1,')))
        # Explicit reset must freeze, never silently clear prior evidence.
        m.feed(frame('RL1,B,0'))
        self.assertTrue(m.blocked)
        self.assertEqual(m.latest['failures'],3)

    def test_invalid_policy_and_structure(self):
        m=audit.Model('replay')
        m.feed(frame('RL1,P,4,5000,20,3,20'))
        self.assertFalse(m.compatible)
        for body in ['RL1,S,1','RL1,E,1,1,99,0','RL1,P,-1,5000,20,3,20','RL1,B,4294967296']:
            before=m.errors
            m.feed(frame(body))
            self.assertGreater(m.errors,before)

    def test_private_export(self):
        m=audit.Model('replay'); m.feed(self.fixture())
        values=m.latest.copy()
        values['time']+=1
        values['failures']-=1
        m.feed(frame('RL1,S,'+','.join(str(values[k]) for k in audit.FIELDS)))
        self.assertTrue(m.blocked)

    def test_export_permissions_and_provenance(self):
        m=audit.Model('replay'); m.feed(self.fixture())
        with tempfile.TemporaryDirectory() as directory:
            dest=Path(directory)/'capture.json'
            m.export(dest)
            self.assertEqual(dest.stat().st_mode & 0o777,0o600)
            self.assertIn('jayis1',dest.read_text())
            self.assertIn('replay',dest.read_text())
            with self.assertRaises(FileExistsError): m.export(dest)

    def test_exposure_bound_and_sticky_mask(self):
        for change in ('access_ms','faults'):
            m=audit.Model('live'); m.feed(self.fixture())
            values=m.latest.copy(); values['time']+=1
            values[change] = values[change]+2 if change=='access_ms' else 0
            m.feed(frame('RL1,S,'+','.join(str(values[k]) for k in audit.FIELDS)))
            self.assertTrue(m.blocked or m.errors)
            self.assertEqual(m.latest['time'],5500)

    def test_late_policy_and_reset_limitation(self):
        # Policy is repeated by firmware; missing BOOT is not fatal to attachment.
        m=audit.Model('live')
        m.feed(self.fixture().split(b'\n',1)[1])
        self.assertIsNotNone(m.latest)
        # No persistent boot identity: an idle reboot with forward counters/time
        # is indistinguishable. Keep this limitation explicit, not a false pass.
        values=[100,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0]
        m=audit.Model('live'); m.feed(frame('RL1,P,3,5000,20,3,20'))
        m.feed(frame('RL1,S,'+','.join(map(str,values))))
        values[0]=200
        m.feed(frame('RL1,S,'+','.join(map(str,values))))
        self.assertFalse(m.blocked)  # documented, not authenticated continuity

    def test_serial_constructor_restores_on_flush_failure(self):
        from unittest.mock import patch
        master,slave=pty.openpty()
        original=termios.tcgetattr(slave)
        with patch.object(audit.termios,'tcflush',side_effect=OSError('injected')):
            with self.assertRaises(OSError): audit.Serial(os.ttyname(slave))
        self.assertEqual(original,termios.tcgetattr(slave))
        os.close(master); os.close(slave)

    def test_serial_pty(self):
        master,slave=pty.openpty()
        original=termios.tcgetattr(slave)
        serial=audit.Serial(os.ttyname(slave))
        os.write(master,self.fixture()[:200])
        time.sleep(.02)
        self.assertTrue(serial.read())
        serial.close()
        self.assertEqual(original,termios.tcgetattr(slave))
        serial=audit.Serial(os.ttyname(slave))
        os.close(master)
        with self.assertRaises(OSError): serial.read()
        serial.close(); os.close(slave)

if __name__=='__main__': unittest.main()
