#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Static release gates; run builds and CAD checks separately."""
from pathlib import Path
import re
import subprocess
ROOT=Path(__file__).resolve().parents[1]
files=[p for p in ROOT.rglob('*') if p.is_file() and not any(
    part in ('build','__pycache__') for part in p.relative_to(ROOT).parts)
    and p.suffix not in ('.pyc','.kicad_prl')]
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'Missing authorship: {p}'
    assert not re.search(r'Author\s*:\s*(?:Hermes|Claude|Nous|Anthropic)',text,re.I),p
words=len((ROOT/'README.md').read_text().split())
c_files=list((ROOT/'firmware').rglob('*.c'))
lines=sum(sum(bool(line.strip()) for line in re.sub(r'/\*.*?\*/|//[^\n]*','',
    p.read_text(),flags=re.S).splitlines()) for p in c_files)
assert words>=2000,words
assert lines>=500,lines
assert "['ATTEMPT', 'FAILURE', 'SUCCESS', 'RESET']" in (ROOT/'tools/generate_hardware.py').read_text()
assert 'ATTEMPT PA0; FAILURE PA1; SUCCESS PA4; RESET PB0' in (ROOT/'firmware/board.h').read_text()
subprocess.run(['git','diff','--check'],cwd=ROOT,check=True)
print(f'PASS: {len(files)} attributed files, {words} README words, {lines} nonblank noncomment production C lines')
