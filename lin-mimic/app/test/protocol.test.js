// Protocol tests. Author: jayis1.
import test from 'node:test';import assert from 'node:assert/strict';
import{packet,Parser,CMD,allowMask,crc16,txPayload}from'../src/protocol.js';
test('CRC standard vector',()=>assert.equal(crc16([...new TextEncoder().encode('123456789')]),0x29b1));
test('escaped packet round trip',()=>{let got;const p=new Parser((c,d)=>got=[c,d]);p.feed(packet(CMD.TRANSMIT,[0x7e,0x7d,1]));assert.equal(got[0],CMD.TRANSMIT);assert.deepEqual(got[1],[0x7e,0x7d,1]);});
test('allowlist packs IDs',()=>assert.deepEqual([...allowMask([0,7,8,63])],[129,1,0,0,0,0,0,128]));
test('tx validates shape',()=>assert.deepEqual([...txPayload(0x12,[1,2],1,0)],[0x12,2,0,1,1,2]));
