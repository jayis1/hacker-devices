#!/usr/bin/env python3
# Author: jayis1. SPDX-License-Identifier: MIT
"""Generate the passive Dependency Diamond carrier with KiCad 9."""
import json
from pathlib import Path
import subprocess
import uuid
import pcbnew as pcb
ROOT = Path(__file__).resolve().parents[1] / 'kicad'
ROOT.mkdir(exist_ok=True)
SIGNALS = ['TAG0', 'TAG1', 'START', 'FINISH']
def uid(name):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, 'jayis1/dependency-diamond/' + name))
def point(x, y):
    return pcb.VECTOR2I(pcb.FromMM(x), pcb.FromMM(y))
components = []
def component(ref, value, symbol, footprint, x, y, nets):
    components.append(dict(ref=ref, value=value, symbol=symbol, footprint=footprint,
                           x=x, y=y, nets=nets, uuid=uid(ref)))
header = 'Connector_PinHeader_2.54mm:PinHeader_1x0{}_P2.54mm_Vertical'
resistor = 'Resistor_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P7.62mm_Horizontal'
component('J1', 'GROUND_ONLY', 'Conn_2', header.format(2), 20, 20, ['GND', 'GND'])
component('J2', 'A0_A1_A2_A3', 'Conn_4', header.format(4), 20, 35, SIGNALS)
for i, name in enumerate(SIGNALS):
    y = 35 + i * 12
    component(f'R{2*i+1}', '100k', 'R', resistor, 30, y, ['GND', name])
    component(f'R{2*i+2}', '1k', 'R', resistor, 45, y, [name, name+'_IN'])
    component(f'J{i+3}', name+'_IN', 'Conn_2', header.format(2), 65, y, [name+'_IN', 'GND'])
board = pcb.BOARD()
board.SetCopperLayerCount(2)
board.GetDesignSettings().SetBoardThickness(pcb.FromMM(1.6))
title = pcb.TITLE_BLOCK()
title.SetTitle('Dependency Diamond receive-only carrier')
title.SetComment(0, 'Author: jayis1')
title.SetRevision('A')
board.SetTitleBlock(title)
nets = {}
for name in sorted({n for c in components for n in c['nets']}):
    net = pcb.NETINFO_ITEM(board, name)
    board.Add(net)
    nets[name] = net
for c in components:
    library, name = c['footprint'].split(':')
    fp = pcb.FootprintLoad('/usr/share/kicad/footprints/' + library + '.pretty', name)
    if fp is None:
        raise RuntimeError(c['footprint'])
    fp.SetFPID(pcb.LIB_ID(library, name))
    fp.SetReference(c['ref'])
    fp.SetValue(c['value'])
    fp.SetPosition(point(c['x'], c['y']))
    fp.SetPath(pcb.KIID_PATH('/' + uid('sheet') + '/' + c['uuid']))
    board.Add(fp)
    for pad in fp.Pads():
        pad.SetNet(nets[c['nets'][int(pad.GetNumber()) - 1]])
    fp.Reference().SetTextSize(point(1, 1))
    fp.Reference().SetPosition(point(c['x'] + 3, c['y'] - 2))
    fp.Value().SetVisible(False)
def route(net, layer, coordinates):
    for start, end in zip(coordinates, coordinates[1:]):
        if start == end:
            continue
        segment = pcb.PCB_TRACK(board)
        segment.SetStart(point(*start))
        segment.SetEnd(point(*end))
        segment.SetWidth(pcb.FromMM(0.3))
        segment.SetLayer(layer)
        segment.SetNet(nets[net])
        board.Add(segment)
def via(net, x, y):
    v = pcb.PCB_VIA(board)
    v.SetPosition(point(x, y))
    v.SetWidth(pcb.F_Cu, pcb.FromMM(0.8))
    v.SetDrill(pcb.FromMM(0.4))
    v.SetViaType(pcb.VIATYPE_THROUGH)
    v.SetLayerPair(pcb.F_Cu, pcb.B_Cu)
    v.SetNet(nets[net])
    board.Add(v)
route('GND', pcb.B_Cu, [(20,22.54),(20,20),(30,20),(30,35),(30,47),(30,59),(30,71)])
route('GND', pcb.F_Cu, [(20,22.54),(80,22.54)])
via('GND',80,22.54)
route('GND',pcb.B_Cu,[(80,22.54),(80,37.54),(80,49.54),(80,61.54),(80,73.54)])
for i,name in enumerate(SIGNALS):
    y = 35 + i * 12
    x = 27 - i * 1.5
    route(name,pcb.F_Cu,[(20,35+i*2.54),(x,35+i*2.54),(x,y-3),(37.62,y-3),(37.62,y),(45,y)])
    route(name+'_IN',pcb.F_Cu,[(52.62,y),(65,y)])
    route('GND',pcb.B_Cu,[(65,y+2.54),(80,y+2.54)])
for start,end in [((10,10),(90,10)),((90,10),(90,80)),((90,80),(10,80)),((10,80),(10,10))]:
    edge = pcb.PCB_SHAPE()
    edge.SetShape(pcb.SHAPE_T_SEGMENT)
    edge.SetStart(point(*start))
    edge.SetEnd(point(*end))
    edge.SetWidth(pcb.FromMM(0.05))
    edge.SetLayer(pcb.Edge_Cuts)
    board.Add(edge)
for text,x,y in [('Dependency Diamond A / jayis1',50,14),('3V3 ONLY - NO ISOLATION',53,18),
                  ('J1: BOTH PINS GND',35,26),('J2: 1=A0 2=A1 3=A2 4=A3',45,77)]:
    item = pcb.PCB_TEXT(board)
    item.SetText(text)
    item.SetPosition(point(x,y))
    item.SetTextSize(point(1,1))
    item.SetTextThickness(pcb.FromMM(0.15))
    item.SetLayer(pcb.F_SilkS)
    board.Add(item)
pcb.SaveBoard(str(ROOT/'device.kicad_pcb'),board)
def symbol(name,count):
    prefix = 'R' if name == 'R' else 'J'
    bottom = -1.27 if name == 'R' else -2.54*(count-1)-1.27
    graphics = f'(rectangle (start -2.54 1.27) (end 2.54 {bottom}) (stroke (width 0.254) (type default)) (fill (type none)))'
    pins=[]
    for i in range(count):
        x,y,angle = (-5.08,0,0) if name=='R' and i==0 else ((5.08,0,180) if name=='R' else (-5.08,-2.54*i,0))
        pins.append(f'(pin passive line (at {x} {y} {angle}) (length 2.54) (name "{i+1}" (effects (font (size 1.27 1.27)))) (number "{i+1}" (effects (font (size 1.27 1.27)))))')
    return f'''(symbol "{name}" (pin_names (offset 0.5)) (in_bom yes) (on_board yes)
(property "Reference" "{prefix}" (at 0 3.81 0) (effects (font (size 1.27 1.27))))
(property "Value" "{name}" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
(property "Author" "jayis1" (at 0 0 0) (effects (font (size 1.27 1.27)) hide))
(symbol "{name}_0_1" {graphics})(symbol "{name}_1_1" {''.join(pins)}))'''
symbols=[symbol('R',2),symbol('Conn_2',2),symbol('Conn_4',4)]
(ROOT/'Witness.kicad_sym').write_text('(kicad_symbol_lib (version 20231120) (generator "jayis1")\n'+'\n'.join(symbols)+')\n')
(ROOT/'sym-lib-table').write_text('(sym_lib_table (version 7) (lib (name "Witness")(type "KiCad")(uri "${KIPRJMOD}/Witness.kicad_sym")(options "")(descr "Author: jayis1")))\n')
embedded='\n'.join(s.replace('(symbol "','(symbol "Witness:',1) for s in symbols)
parts=[f'''(kicad_sch (version 20250114) (generator "eeschema")
(uuid "{uid('sheet')}") (paper "A4")
(title_block (title "Dependency Diamond receive-only carrier") (rev "A") (comment 1 "Author: jayis1"))
(lib_symbols {embedded})''']
for index,c in enumerate(components):
    if index<2:
        x,y=45.72,35.56+index*25.4
    else:
        row,column=divmod(index-2,3)
        x,y=101.6+column*50.8,35.56+row*38.1
    parts.append(f'''(symbol (lib_id "Witness:{c['symbol']}") (at {x} {y} 0) (unit 1)
(in_bom yes) (on_board yes) (dnp no) (uuid "{c['uuid']}")
(property "Reference" "{c['ref']}" (at {x} {y-6} 0) (effects (font (size 1.27 1.27))))
(property "Value" "{c['value']}" (at {x} {y-3} 0) (effects (font (size 1.27 1.27))))
(property "Footprint" "{c['footprint']}" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))
(property "Author" "jayis1" (at {x} {y} 0) (effects (font (size 1.27 1.27)) hide))
(instances (project "device" (path "/{uid('sheet')}" (reference "{c['ref']}") (unit 1)))))''')
    for i,net in enumerate(c['nets']):
        px=x+(5.08 if c['symbol']=='R' and i==1 else -5.08)
        py=y if c['symbol']=='R' else y+i*2.54
        ex=px+(5.08 if c['symbol']=='R' and i==1 else -5.08)
        parts.append(f'(wire (pts (xy {px} {py})(xy {ex} {py})) (stroke (width 0) (type default)) (uuid "{uid(c["ref"]+str(i)+"wire")}"))')
        justify = 'right' if ex < px else 'left'
        parts.append(f'(label "{net}" (at {ex} {py} 0) (effects (font (size 1 1)) (justify {justify} bottom)) (uuid "{uid(c["ref"]+str(i)+"label")}"))')
parts.append(')')
(ROOT/'device.kicad_sch').write_text('\n'.join(parts)+'\n')
(ROOT/'device.kicad_pro').write_text(json.dumps({'meta':{'filename':'device.kicad_pro','version':1},
 'text_variables':{'AUTHOR':'jayis1'},'net_settings':{'classes':[{'name':'Default','clearance':0.2,
 'track_width':0.3,'via_diameter':0.8,'via_drill':0.4}],'version':3}},indent=2)+'\n')
(ROOT/'connectivity.json').write_text(json.dumps({'author':'jayis1','components':components},indent=2)+'\n')
subprocess.run(['kicad-cli','sch','export','netlist','--format','kicadxml',
                '-o',str(ROOT/'device.net'),str(ROOT/'device.kicad_sch')],check=True)
print(f'Generated {len(components)} components, {len(nets)} nets')
