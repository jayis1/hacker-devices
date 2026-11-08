# Author: jayis1. SPDX-License-Identifier: GPL-2.0-only
import itertools
import json
import os
from pathlib import Path
import pty
import subprocess
import sys
import tempfile
import termios
import time
import tkinter as tk
import unittest
from unittest.mock import patch
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'app'))
from console import Console, Model, frame, read_replay, Serial
from contract import FIELDS, POLICY, validate, transition, eligible, references


def baseline():
    s = dict.fromkeys(FIELDS, 0)
    s.update(now=20, valid=15, phase=1, stable=5, observed=17)
    return s


def snapshot(ops):
    # Independent accounting builder, not a call to the production engine.
    s = baseline()
    for tag, retain in ops:
        s['now'] += 24
        s['observed'] += 24
        s['attempts'] += 1
        if retain:
            s['queued'] += bool(s['active'])
            s['active'] |= 1 << tag
            s['launched'] += 1
            s['use'+str(tag)] += 1
            s['stamp'+str(tag)] = s['launched']
            s['ref'+str(tag)] += 1
            s['depth'] += 1
            s['peak'] = max(s['peak'], s['depth'])
        else:
            s['ref'+str(tag)] -= 1
            if not s['ref'+str(tag)]: s['active'] &= ~(1 << tag)
            s['depth'] -= 1
            s['completed'] += 1
            s['rounds'] += not s['active']
    return s


def stream(*states):
    return frame('P', POLICY) + b''.join(frame('S', [s[k] for k in FIELDS]) for s in states)


class ContractTests(unittest.TestCase):
    def test_c_fixtures(self):
        for mode, fault, depth in [('legal', 0, 0), ('underflow', 2, 0), ('ceiling', 1, 3)]:
            with self.subTest(mode=mode):
                m = Model('synthetic C waveform', 'replay')
                m.feed(subprocess.check_output([ROOT/'firmware/build/fixture', mode]))
                m.finish()
                self.assertEqual((m.errors, m.frozen), (0, ''))
                self.assertEqual((m.latest['faults'], m.latest['depth']), (fault, depth))

    def test_all_reference_vectors(self):
        for refs in itertools.product(range(4), repeat=4):
            ops = [(tag, True) for tag, n in enumerate(refs) for _ in range(n)]
            s = snapshot(ops)
            validate(s)
            self.assertEqual(references(s), list(refs))
            self.assertEqual(eligible(s), [i for i, n in enumerate(refs) if n < 3])

    def test_partial_drop_and_retain(self):
        ops = [(0, True), (0, True), (0, False)]
        a = snapshot(ops)
        b = snapshot(ops + [(0, True)])
        validate(a); validate(b)
        self.assertEqual(a['active'], 1)
        self.assertEqual(transition(a, b, 24), '')

    def test_reference_migration(self):
        a = snapshot([(0, True), (0, True), (1, True)])
        b = dict(a, now=a['now']+100, ref0=1, ref1=2)
        # Locally plausible counts, but a release without retain cannot move a reference.
        b['use1'] = 2; b['use0'] = 1
        b['stamp0'] = 1; b['stamp1'] = 3
        # Locally valid, but its cumulative retain history moved across objects.
        validate(b)
        m = Model('CRC-valid retain migration', 'replay')
        m.feed(stream(a, b)); self.assertTrue(m.frozen)
        a = snapshot([(0, True), (1, True), (1, True), (1, False), (0, True)])
        b = dict(a, now=a['now']+100, ref0=1, ref1=2)
        validate(a); validate(b)
        m = Model('CRC-valid reference migration', 'replay')
        m.feed(stream(a, b)); self.assertTrue(m.frozen)

    def test_sparse_legitimate_reuse(self):
        ops = [(0, True), (0, True)]
        a = snapshot(ops)
        b = snapshot(ops+[(0, False), (0, False), (1, True), (1, False), (0, True)])
        validate(b); self.assertEqual(transition(a, b, 120), '')

    def test_fault_plausibility(self):
        for reason, ops in [(1, []), (2, [(0, True)])]:
            s = snapshot(ops)
            s.update(levels=4 if reason == 1 else 8, halted=1, valid=0, faults=reason, rejected=1)
            s['attempts'] += 1
            m = Model('impossible semantic fault', 'replay'); m.feed(stream(s))
            self.assertEqual(m.errors, 1); self.assertIsNone(m.latest)

    def test_invalid_counts(self):
        for key, value in [('active', 1), ('ref0', 4), ('use0', 1), ('depth', 1),
                           ('peak', 13), ('rounds', 1), ('faults', 4), ('now', True), ('stamp0', 1)]:
            with self.subTest(key=key):
                s = baseline(); s[key] = value
                with self.assertRaises(ValueError): validate(s)

    def test_stamp_migration(self):
        a = snapshot([(0, True), (1, True)])
        b = dict(a, now=a['now']+100, stamp0=2, stamp1=1)
        validate(b); self.assertTrue(transition(a, b, 100))

    def test_halt_freeze(self):
        a = snapshot([(0, True)]*3)
        a.update(levels=4, halted=1, valid=0, faults=1, rejected=1)
        a['attempts'] += 1
        b = dict(a, now=a['now']+100, observed=a['observed']+100)
        m = Model('test', 'replay'); m.feed(stream(a, b))
        self.assertIn('Halted evidence', m.frozen)

    def test_parser_recovery_tail_policy_reset(self):
        m = Model('test', 'replay')
        m.feed(b'x'*900+b'AD1,S,1'+stream(baseline())); m.finish()
        self.assertGreater(m.errors, 0); self.assertEqual(m.latest, baseline())
        n = m.errors; m.feed(b'AD1,S,1*0000\n'); self.assertGreater(m.errors, n)
        m.feed(b'AD1,S'); n = m.errors; m.finish(); self.assertEqual(m.errors, n+1)
        m.feed(frame('B', [42])); self.assertTrue(m.frozen)
        m = Model('test', 'replay'); m.feed(frame('P', [3, 1, 5, 4, 2, 1])); self.assertTrue(m.frozen)

    def test_live_stale_duplicate_and_pause(self):
        m = Model('test', 'live'); m.connected = True
        with patch('console.time.monotonic', return_value=10): m.feed(stream(baseline()))
        b = dict(baseline(), now=120, observed=117)
        with patch('console.time.monotonic', return_value=10.1):
            m.feed(stream(b)); self.assertTrue(m.live())
        with patch('console.time.monotonic', return_value=12):
            m.feed(stream(b)); self.assertFalse(m.live())
            m.feed(stream(dict(b, now=220, observed=217))); self.assertTrue(m.frozen)

    def test_drift_and_rollback(self):
        m = Model('synthetic slow delivery', 'live'); m.connected = True
        for i in range(200):
            with patch('console.time.monotonic', return_value=10+i*.11):
                m.feed(stream(dict(baseline(), now=20+i*100, observed=17+i*100)))
            if m.frozen: break
        self.assertIn('progress diverged', m.frozen)
        m = Model('synthetic rollback', 'replay')
        a = snapshot([(0, True), (0, False)])
        b = dict(baseline(), now=a['now']+100, observed=a['observed']+100)
        m.feed(stream(a, b)); self.assertIn('rollback', m.frozen)

    def test_private_export_and_input_bounds(self):
        with tempfile.TemporaryDirectory() as d:
            p = Path(d)/'report.json'; m = Model('test', 'replay'); m.export(p)
            self.assertEqual(p.stat().st_mode & 0o777, 0o600)
            with self.assertRaises(FileExistsError): m.export(p)
            p = Path(d)/'fifo'; os.mkfifo(p)
            with self.assertRaises(ValueError): read_replay(p)
            p = Path(d)/'huge'; p.write_bytes(b'x'*(2*1024*1024+1))
            with self.assertRaises(ValueError): read_replay(p)
        with self.assertRaises((OSError, termios.error)): Serial('/dev/null')
        master, slave = pty.openpty(); old = termios.tcgetattr(slave)
        try:
            with patch('console.termios.tcflush', side_effect=OSError('injected')):
                with self.assertRaises(OSError): Serial(os.ttyname(slave))
            self.assertEqual(old, termios.tcgetattr(slave))
        finally: os.close(master); os.close(slave)


class GuiTests(unittest.TestCase):
    def test_c_fixture_screens_and_export(self):
        root = tk.Tk(); app = Console(root)
        try:
            with tempfile.TemporaryDirectory() as d:
                for mode, reason in [('legal', ''), ('underflow', 'reference underflow'),
                                     ('ceiling', 'reference ceiling exceeded')]:
                    p = Path(d)/mode
                    p.write_bytes(subprocess.check_output([ROOT/'firmware/build/fixture', mode]))
                    app.load(p); root.update()
                    self.assertEqual((app.model.errors, app.model.frozen), (0, ''))
                    self.assertIn(reason, app.summary.get('1.0', 'end'))
                    self.assertIn('Reference multiplicities', app.summary.get('1.0', 'end'))
                    self.assertEqual(app.history.get('1.0', 'end').count("'now':"), len(app.model.history))
                for tab in app.tabs.tabs(): app.tabs.select(tab); root.update()
                out = Path(d)/'export.json'
                with patch('console.filedialog.asksaveasfilename', return_value=str(out)): app.export()
                saved = json.loads(out.read_text())
                self.assertEqual(saved['latest']['ref3'], 3)
                self.assertEqual(saved['author'], 'jayis1'); self.assertFalse(saved['live'])
                with patch('console.filedialog.asksaveasfilename', return_value=str(out)): app.export()
                self.assertNotIn('Saved private', app.message)
                retained = app.model
                with patch('console.os.open', side_effect=PermissionError('denied')): app.connect()
                self.assertIs(app.model, retained)
        finally: app.close(); root.destroy()

    def test_pty_read_double_connect_restore_and_hangup(self):
        root = tk.Tk(); app = Console(root)
        master, slave = pty.openpty(); old = termios.tcgetattr(slave)
        try:
            app.path.set(os.ttyname(slave)); app.connect()
            os.write(master, stream(snapshot([(0, True), (0, True)])))
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertEqual(app.model.latest['ref0'], 2)
            self.assertIn('Reference multiplicities: [2, 0, 0, 0]', app.summary.get('1.0', 'end'))
            self.assertEqual(len(app.tabs.tabs()), 3)
            app.connect(); self.assertIn('Disconnect', app.message)
            app.disconnect(); self.assertEqual(old, termios.tcgetattr(slave))
            self.assertEqual(app.model.latest['ref0'], 2)
            app.connect(); os.close(master); master = None
            for _ in range(10): root.update(); time.sleep(.025)
            self.assertFalse(app.model.connected)
        finally:
            app.close(); root.destroy(); os.close(slave)
            if master is not None: os.close(master)


if __name__ == '__main__': unittest.main()
