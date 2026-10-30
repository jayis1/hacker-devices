#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Supplement executable tests with length, authorship and scope gates."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
words=len((root/'README.md').read_text().split())
assert words>=2000,words
lines=0
for p in (root/'firmware').rglob('*.c'):
    source=re.sub(r'/\*.*?\*/|//[^\n]*','',p.read_text(),flags=re.S)
    lines+=sum(bool(line.strip()) for line in source.splitlines())
assert lines>=500,lines
files=[p for p in root.rglob('*') if p.is_file() and not any(x in p.parts for x in ('build','__pycache__')) and p.suffix!='.kicad_prl']
for p in files:
    text=p.read_text()
    assert 'jayis1' in text, f'No author: {p}'
    assert not re.search(r'^.*Author: (?:Hermes|Claude|Anthropic|Nous Research)',text,re.I|re.M),p
    assert not re.search(r'-----BEGIN (?:RSA |OPENSSH )?PRIVATE KEY-----',text),p
for name in ['device.kicad_sch','device.kicad_pcb','device.kicad_pro']:
    assert (root/'kicad'/name).is_file()
subprocess.run(['git','diff','--check'],cwd=root,check=True)
print(f'PASS: {words} README words; {lines} substantive production C lines; {len(files)} attributed deliverables')
