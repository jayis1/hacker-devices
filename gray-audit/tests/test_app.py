# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import binascii
import importlib.util
import pathlib
import tempfile
import unittest

PATH = pathlib.Path(__file__).resolve().parents[1] / 'app' / 'gray.py'
spec = importlib.util.spec_from_file_location('gray', PATH)
assert spec is not None and spec.loader is not None
gray = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gray)


def frame(payload):
    raw = payload.encode('ascii')
    return raw + f'*{binascii.crc_hqx(raw, 0xffff):04X}\n'.encode()


class Protocol(unittest.TestCase):
    def test_crc_and_fields(self):
        p = gray.parse(frame('GA1,H,1,2,3,4,5,6,7'))
        self.assertEqual(p, ('H', [1, 2, 3, 4, 5, 6, 7]))
        for raw in [b'bad\n', frame('GA1,H,1'), frame('GA1,H,-1,2,3,4,5,6,7'),
                    frame('GA1,E,1,2,3,4,0,0'), b'X'*400,
                    frame('GA1,H,1,2,3,4,5,6,7').replace(b'*', b'*0')]:
            with self.assertRaises(ValueError):
                gray.parse(raw)

    def test_stream(self):
        s = gray.Stream()
        good = frame('GA1,H,1,2,3,4,5,6,7')
        self.assertEqual(s.feed(good[:5]), [])
        self.assertEqual(s.feed(good[5:]), [('H', [1,2,3,4,5,6,7])])
        self.assertEqual(s.feed(b'X'*1000 + b'\n' + good), [('H', [1,2,3,4,5,6,7])])
        self.assertEqual(s.errors, 1)

    def test_reset_and_wrap(self):
        m = gray.Model()
        m.accept('H', [4294967295, 1, 0, 0, 0, 0, 0], 1)
        m.accept('H', [0, 1, 0, 0, 0, 0, 0], 2)
        self.assertEqual(m.resets, 0)
        m.accept('H', [100, 1, 0, 0, 0, 0, 0], 3)
        m.accept('E', [50, 0, 0, 1, 0, 3], 4)  # queued event is older
        self.assertEqual(m.resets, 0)
        m.accept('H', [2, 1, 0, 0, 0, 0, 0], 5)
        self.assertEqual(m.resets, 1)
        self.assertEqual(len(m.events), 0)

    def test_private_export(self):
        with tempfile.TemporaryDirectory() as d:
            p = pathlib.Path(d) / 'capture.json'
            m = gray.Model()
            m.accept('H', [1,2,3,4,5,6,7], 1)
            gray.export(m, p, 4, 'disconnected', 2)
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            self.assertIn('jayis1', p.read_text())
            with self.assertRaises(FileExistsError):
                gray.export(m, p, 5, 'connected', 0)


if __name__ == '__main__':
    unittest.main()
