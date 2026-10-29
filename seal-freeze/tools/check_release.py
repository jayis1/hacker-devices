#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Release shape and attribution checks; builds/CAD checks run separately."""
from pathlib import Path
import re
import subprocess
root = Path(__file__).resolve().parents[1]
words = len((root/'README.md').read_text().split())
assert words >= 2000, words
sources = list((root/'firmware').rglob('*.c'))
text = '\n'.join(p.read_text() for p in sources)
stripped = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
lines = sum(bool(line.strip()) for line in stripped.splitlines())
assert lines >= 500, lines
required = ['firmware/main.c','firmware/board.h','firmware/registers.h','firmware/Makefile',
            'app/console.py','kicad/device.kicad_sch','kicad/device.kicad_pcb','kicad/device.kicad_pro']
assert all((root/name).is_file() for name in required)
files = subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard','--',str(root)],text=True).splitlines()
for name in files:
    p = Path(name)
    assert 'jayis1' in p.read_text(), f'missing author: {name}'
    assert p.stat().st_size, name
    assert p.suffix not in ('.elf','.bin','.pyc','.map','.kicad_prl')
print(f'PASS: {words} README words; {lines} nonblank, noncomment production C lines; {len(files)} attributed files')
