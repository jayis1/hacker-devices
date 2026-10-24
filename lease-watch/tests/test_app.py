# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import importlib.util
from pathlib import Path
import tempfile
import unittest
import random

SPEC = importlib.util.spec_from_file_location('lease_app', Path(__file__).resolve().parents[1] / 'app/lease.py')
assert SPEC is not None and SPEC.loader is not None
app = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(app)

def packet(text):
    raw = text.encode('ascii')
    return raw + f'*{app.crc(raw):04X}\n'.encode('ascii')

class ProtocolTests(unittest.TestCase):
    def test_bounded_random_input(self):
        rng = random.Random(401)
        stream = app.Framer()
        for _ in range(10000):
            data = rng.randbytes(rng.randrange(512))
            for frame in stream.feed(data):
                try:
                    app.parse(frame)
                except ValueError:
                    pass
            self.assertLessEqual(len(stream.buffer), 383)

    def test_crc_vector(self):
        self.assertEqual(app.crc(b'123456789'), 0x29B1)

    def test_strict_parser(self):
        self.assertEqual(app.parse(packet('LW1,B,0,250,3')), ('B', [0, 250, 3]))
        for text in ('LW1,B,-1,250,3', 'LW2,B,0,250,3', 'LW1,E,1,2,8,0',
                     'LW1,E,1,2,1,16', 'LW1,B,0,251,3', 'LW1,B,4294967296,250,3'):
            with self.assertRaises(ValueError): app.parse(packet(text))
        with self.assertRaises(ValueError): app.parse(b'LW1,B,0,250,3*0000\n')

    def test_stream_resynchronizes(self):
        stream = app.Framer()
        self.assertEqual(stream.feed(b'x' * 1000), [])
        self.assertEqual(stream.feed(b'\n' + packet('LW1,B,0,250,3')), [packet('LW1,B,0,250,3')])
        self.assertEqual(stream.rejected, 1)

    def test_freshness_and_reboot(self):
        m = app.Model()
        row = [100, 14, 1, 4294967295] + [0] * 14
        m.accept(('S', row), 10)
        self.assertFalse(m.fresh(10.2))
        m.connected = True
        self.assertTrue(m.fresh(10.2))
        self.assertFalse(m.fresh(11.1))
        m.accept(('E', [0, 1, 1, 12]), 10.3)
        self.assertEqual(m.sessions, 1)  # queued event is not a reboot
        m.accept(('S', [50] + row[1:]), 10.4)
        self.assertEqual(m.sessions, 2)
        m.accept(('B', [0, 250, 3]), 10.5)
        self.assertIsNone(m.status)

    def test_export_private_no_overwrite(self):
        m = app.Model()
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / 'report.json'
            m.export(path, 1)
            self.assertEqual(path.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): m.export(path, 1)

if __name__ == '__main__': unittest.main()
