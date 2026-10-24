#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Compare generated schematic and board, including actual library identifiers."""
import json
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import pcbnew

root = Path(__file__).resolve().parents[1]
kicad = root / 'kicad'
expected = {c['ref']: c for c in json.loads((kicad/'connectivity.json').read_text())['components']}
with tempfile.TemporaryDirectory(prefix='grant-eda-') as directory:
    work = Path(directory)
    for domain, check, suffix in [('sch', 'erc', 'kicad_sch'), ('pcb', 'drc', 'kicad_pcb')]:
        subprocess.run(['kicad-cli', domain, check, '--exit-code-violations', '-o',
                        str(work/(check+'.rpt')), str(kicad/('device.'+suffix))], check=True)
    netlist = work/'device.xml'
    subprocess.run(['kicad-cli', 'sch', 'export', 'netlist', '--format', 'kicadxml',
                    '-o', str(netlist), str(kicad/'device.kicad_sch')], check=True)
    xml = ET.parse(netlist).getroot()
    schematic = {(n.attrib['ref'], n.attrib['pin']): net.attrib['name'].lstrip('/')
                 for net in xml.findall('./nets/net') for n in net.findall('node')}
    values = {c.attrib['ref']: (c.findtext('value'), c.findtext('footprint'))
              for c in xml.findall('./components/comp')}
    assert values == {ref: (c['value'], c['footprint']) for ref, c in expected.items()}
    board = pcbnew.LoadBoard(str(kicad/'device.kicad_pcb'))
    actual = {}
    for fp in board.GetFootprints():
        ref = fp.GetReference()
        assert ref in expected and fp.GetValue() == expected[ref]['value']
        identity = fp.GetFPID()
        assert str(identity.GetLibNickname())+':'+str(identity.GetLibItemName()) == expected[ref]['footprint']
        for pad in fp.Pads():
            actual[ref, pad.GetNumber()] = pad.GetNetname()
    target = {(ref, str(i+1)): net for ref, c in expected.items() for i, net in enumerate(c['nets'])}
    assert actual == target and schematic == target
    print(f'PASS: {len(expected)} components, {len(target)} pins, {len(set(target.values()))} nets')
