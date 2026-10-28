#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Release text checks. Does not replace builds, test execution, ERC or DRC."""
from pathlib import Path
import re
import subprocess

root=Path(__file__).resolve().parents[1]
files=[root/p for p in subprocess.check_output(
    ['git','ls-files','--cached','--others','--exclude-standard', '.'],cwd=root,text=True
).splitlines()]
assert files, 'No deliverables found'
for path in files:
    assert path.is_file() and not path.is_symlink(), path
    text=path.read_text()
    assert 'jayis1' in text, f'Missing author: {path}'
    assert not re.search(r'^\s*(?:#|/\*|\*)?\s*Author[^\n]*(Hermes|Nous Research|Anthropic|Claude)',text,re.I|re.M), path
words=len((root/'README.md').read_text().split())
assert words>=2000, words
lines=0
for path in (root/'firmware').rglob('*.c'):
    text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
    lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500, lines
for path in ['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
             'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro',
             'app/console.py']:
    assert (root/path).is_file(), path
print(f'PASS: {len(files)} authored files; {words} README words; {lines} nonblank, noncomment production C lines (headers/tests excluded)')
