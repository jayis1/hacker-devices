// PTP/1 codec tests
// Author: jayis1
import test from 'node:test'; import assert from 'node:assert/strict';
import { MAGIC, commands, encodeFrame, decodeEvent, patternPayload, armPayload, protocolResponse } from '../src/protocol.js';
import { ProbeTransport } from '../src/transport.js';
test('encodes bounded little-endian command frame',()=>{const frame=encodeFrame(commands.start,7,new Uint8Array([1,2]));const view=new DataView(frame.buffer);assert.equal(view.getUint32(0,true),MAGIC);assert.equal(view.getUint32(4,true),7);assert.equal(view.getUint16(8,true),2);assert.deepEqual([...frame.slice(12)],[1,2]);});
test('rejects oversized payload and invalid sequence',()=>{assert.throws(()=>encodeFrame(1,0),RangeError);assert.throws(()=>encodeFrame(1,1,new Uint8Array(245)),RangeError);});
test('decodes fixed event record',()=>{const bytes=new Uint8Array(16);const view=new DataView(bytes.buffer);view.setBigUint64(0,1234n,true);view.setUint32(8,99,true);view.setUint16(12,4,true);bytes[14]=2;bytes[15]=3;assert.deepEqual(decodeEvent(bytes),{timestamp:1234,value:99,detail:4,type:2,channel:3});});
test('encodes bounded fixed-pattern request',()=>{const payload=patternPayload(2,5,250);const view=new DataView(payload.buffer);assert.deepEqual([...payload.slice(0,2)],[2,5]);assert.equal(view.getUint32(2,true),250);assert.throws(()=>patternPayload(4,1,1),RangeError);assert.throws(()=>patternPayload(0,0,1),RangeError);assert.throws(()=>patternPayload(0,1,251),RangeError);});
test('accepts unsigned sequence values with high bit set',()=>{const frame=encodeFrame(commands.status,0xffffffff);assert.equal(new DataView(frame.buffer).getUint32(4,true),0xffffffff);});
test('encodes deterministic arm challenge',()=>{const nonce=0xa5c31f27;const payload=armPayload(nonce);const view=new DataView(payload.buffer);assert.equal(view.getUint32(0,true),nonce);assert.equal(view.getUint32(4,true),protocolResponse(nonce));});
test('mock transport acknowledges and records frames',async()=>{const transport=new ProbeTransport(true);const info=await transport.connect();const frame=encodeFrame(commands.status,1);const result=await transport.send(frame);assert.equal(info.mock,true);assert.equal(result.status,0);assert.equal(transport.history.length,1);assert.deepEqual([...transport.history[0]],[...frame]);});
