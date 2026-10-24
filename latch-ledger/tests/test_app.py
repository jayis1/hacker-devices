# Author: jayis1. SPDX-License-Identifier: MIT
import importlib.util
import pathlib
import unittest

PATH = pathlib.Path(__file__).parents[1] / 'app' / 'ledger.py'
spec = importlib.util.spec_from_file_location('ledger', PATH)
assert spec is not None and spec.loader is not None
ledger = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ledger)


def frame(body):
    return f'{body}*{ledger.crc(body.encode()):04X}\n'.encode()


class ProtocolTests(unittest.TestCase):
    def test_crc(self):
        self.assertEqual(ledger.crc(b'123456789'), 0x29B1)

    def test_partial_and_invalid(self):
        decoder = ledger.Decoder()
        packet = frame('LL1,H,10,0,0,0,0')
        self.assertEqual(decoder.feed(packet[:7]), [])
        self.assertEqual(decoder.feed(packet[7:])[0][0], 'H')
        self.assertEqual(decoder.feed(b'x' * 500 + b'\n' + packet)[0][0], 'H')
        self.assertEqual(decoder.errors, 1)
        self.assertEqual(decoder.feed(packet.replace(b'10', b'20')), [])
        self.assertEqual(decoder.errors, 2)

    def test_bounds(self):
        d = ledger.Decoder()
        for body in ['LL1,S,1,4,0,0,0,0,0,0,0', 'LL1,H,-1,0,0,0,0',
                     'LL2,H,1,0,0,0,0', 'LL1,E,1,2,8,0,0,0',
                     'LL1,H,4294967296,0,0,0,0', 'LL1,B,0,9,1']:
            self.assertEqual(d.feed(frame(body)), [])
        self.assertEqual(d.errors, 6)

    def test_sessions_and_gaps(self):
        m = ledger.Model()
        m.accept(('E', [1, 10, 1, 0, 1, 5]))
        m.accept(('E', [3, 12, 1, 0, 0, 2]))
        self.assertEqual(m.gaps, 1)
        m.accept(('B', [0, 5, 1]))
        self.assertEqual(m.sessions, 1)
        self.assertIsNone(m.sequence)
        self.assertEqual(len(m.events), 0)

    def test_lost_boot_and_freshness(self):
        m = ledger.Model()
        m.accept(('H', [1000, 2, 0, 0, 0]))
        m.accept(('S', [1000, 0, 1, 1, 0, 0, 0, 0, 0]))
        self.assertIn('channel_age_seconds', m.snapshot(0))
        m.accept(('H', [20, 0, 0, 0, 0]))
        self.assertEqual(m.channels, {})
        self.assertEqual(m.inferred_restarts, 1)
        m.accept(('H', [0xFFFFFFFE, 0, 0, 0, 0]))
        m.accept(('H', [5, 0, 0, 0, 0]))
        self.assertEqual(m.inferred_restarts, 1)


if __name__ == '__main__':
    unittest.main()
