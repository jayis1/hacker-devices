#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Check release scope, attribution and minimum substantive source size."""
from pathlib import Path
import re
import subprocess
ROOT = Path(__file__).resolve().parents[1]
words = len((ROOT / 'README.md').read_text().split())
assert words >= 2000, words
counts = {}
for suffix in ('.c', '.h'):
    counts[suffix] = 0
    for p in (ROOT / 'firmware').rglob('*' + suffix):
        source = re.sub(r'/\*.*?\*/|//[^\n]*', '', p.read_text(), flags=re.S)
        counts[suffix] += sum(bool(line.strip()) for line in source.splitlines())
assert counts['.c'] >= 500, counts
files = subprocess.check_output(['git', 'ls-files', '--cached', '--others', '--exclude-standard', '--', str(ROOT)], text=True).splitlines()
assert files
for name in files:
    p = Path(name)
    if not p.is_absolute():
        p = Path(subprocess.check_output(['git','rev-parse','--show-toplevel'], text=True).strip()) / p
    assert not p.is_symlink(), p
    text = p.read_text()
    assert 'jayis1' in text, p
    assert not re.search(r'-{5}BEGIN [A-Z ]*PRIVATE KEY-{5}|(?:api_key|password|secret)\s*=\s*[\"\'][^\"\']{6,}', text, re.I), p
for name in ('main.c', 'board.h', 'registers.h', 'Makefile'):
    assert (ROOT / 'firmware' / name).is_file()
for suffix in ('kicad_sch', 'kicad_pcb', 'kicad_pro'):
    assert (ROOT / 'kicad' / ('device.' + suffix)).is_file()
print(f'PASS: {words} README words; {counts[".c"]} substantive C lines; {counts[".h"]} header lines; {len(files)} attributed files')
