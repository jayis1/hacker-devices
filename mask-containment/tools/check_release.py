#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Release source inventory; does not replace builds, CAD or physical checks."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
required=['README.md','VERIFICATION.md','firmware/main.c','firmware/board.h',
          'firmware/registers.h','firmware/Makefile','app/console.py','app/contract.py',
          'kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro']
for name in required:
    assert (ROOT/name).is_file(), name
words=len((ROOT/'README.md').read_text().split())
assert words>=2000, words
counts={}
for pattern in ('*.c','*.h'):
    counts[pattern]=0
    for path in (ROOT/'firmware').rglob(pattern):
        text=re.sub(r'/\*.*?\*/|//[^\n]*','',path.read_text(),flags=re.S)
        counts[pattern]+=sum(bool(line.strip()) for line in text.splitlines())
assert counts['*.c']>=500, counts
files=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard',str(ROOT)],cwd=ROOT).decode().splitlines()
assert files
for name in files:
    path=Path(name)
    if not path.is_absolute(): path=ROOT/name
    text=path.read_text()
    assert 'jayis1' in text, path
    assert not re.search(r'^(?:#|/\*)?\s*Author[^\n]*(?:Hermes|Claude|Nous|Anthropic)',text,re.I|re.M),path
print(f'PASS: {words} README words; {counts["*.c"]} nonblank noncomment C lines; '
      f'{counts["*.h"]} header lines; {len(files)} authored deliverables')
