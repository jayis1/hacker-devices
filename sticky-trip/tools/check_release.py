#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Mechanical release requirements; real builds and CAD checks run separately."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
required=['README.md','VERIFICATION.md','firmware/main.c','firmware/board.h',
          'firmware/registers.h','firmware/Makefile','app/console.py',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro']
for name in required:
    assert (root/name).is_file(),name
words=len((root/'README.md').read_text().split())
assert words>=2000,words
lines=0
for p in (root/'firmware').rglob('*.c'):
    text=re.sub(r'/\*.*?\*/','',p.read_text(),flags=re.S)
    lines+=sum(bool(line.strip()) for line in text.splitlines())
assert lines>=500,lines
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(root)],text=True).splitlines()
paths=sorted(set(paths))
for name in paths:
    p=Path(name)
    assert p.is_file() and not p.is_symlink(),name
    assert 'jayis1' in p.read_text(),name
print(f'PASS: {words} README words, {lines} nonblank noncomment C-source lines, {len(paths)} attributed files')
