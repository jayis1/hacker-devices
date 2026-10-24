#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Validate ERC/DRC and actual exported schematic-to-PCB pin connectivity."""
import json
import pathlib
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import pcbnew

root = pathlib.Path(__file__).resolve().parents[1]
kicad = root / 'kicad'
manifest = json.loads((kicad / 'connectivity.json').read_text())
expected = {c['ref']: c for c in manifest['components']}
with tempfile.TemporaryDirectory(prefix='latch-eda-') as temporary:
    temporary = pathlib.Path(temporary)
    for kind, command, extension in [('sch', 'erc', 'kicad_sch'), ('pcb', 'drc', 'kicad_pcb')]:
        subprocess.run(['kicad-cli', kind, command, '--exit-code-violations',
                        '-o', str(temporary / (command + '.rpt')),
                        str(kicad / ('device.' + extension))], check=True)
    netlist = temporary / 'device.xml'
    subprocess.run(['kicad-cli', 'sch', 'export', 'netlist', '--format', 'kicadxml',
                    '-o', str(netlist), str(kicad / 'device.kicad_sch')], check=True)
    xml = ET.parse(netlist).getroot()
    schematic = {}
    for net in xml.findall('./nets/net'):
        for node in net.findall('node'):
            schematic[node.attrib['ref'], node.attrib['pin']] = net.attrib['name'].lstrip('/')
    board = pcbnew.LoadBoard(str(kicad / 'device.kicad_pcb'))
    actual = {}
    for footprint in board.GetFootprints():
        ref = footprint.GetReference()
        assert ref in expected, ref
        assert footprint.GetValue() == expected[ref]['value'], ref
        for pad in footprint.Pads():
            actual[ref, pad.GetNumber()] = pad.GetNetname()
    target = {(ref, str(i + 1)): net for ref, c in expected.items()
              for i, net in enumerate(c['nets'])}
    assert actual == target, (actual, target)
    assert schematic == target, (schematic, target)
    print(f'PASS: {len(expected)} components, {len(target)} pins, '
          f'{len(set(target.values()))} nets agree across schematic, PCB and manifest')
