// LIN Mimic wire protocol. Author: jayis1.
export const SOF=0x7e, ESC=0x7d;
export const CMD={STATUS:1,UNLOCK:2,MODE:3,ALLOW_IDS:4,TRANSMIT:5,LOCK:6};
export function crc16(bytes){let crc=0xffff;for(const byte of bytes){crc^=byte<<8;for(let i=0;i<8;i++)crc=crc&0x8000?((crc<<1)^0x1021)&0xffff:(crc<<1)&0xffff;}return crc;}
export function packet(command,payload=[]){const body=[command,payload.length,...payload];const crc=crc16(body);body.push(crc&255,crc>>8);const wire=[SOF];for(const b of body)b===SOF||b===ESC?wire.push(ESC,b^0x20):wire.push(b);return new Uint8Array(wire);}
export class Parser{
 constructor(onPacket){this.onPacket=onPacket;this.buffer=[];this.escaped=false;}
 feed(bytes){for(let b of bytes){if(b===SOF){this.buffer=[];this.escaped=false;continue;}if(b===ESC){this.escaped=true;continue;}if(this.escaped){b^=0x20;this.escaped=false;}this.buffer.push(b);if(this.buffer.length>=2){const n=this.buffer[1];if(n>88){this.buffer=[];continue;}if(this.buffer.length===n+4){const supplied=this.buffer[n+2]|this.buffer[n+3]<<8;const raw=this.buffer.slice(0,n+2);if(crc16(raw)===supplied)this.onPacket(raw[0],raw.slice(2));this.buffer=[];}}}}
}
export function decodeFrame(p){if(p.length<9)return null;const timestamp=(p[0]|p[1]<<8|p[2]<<16|p[3]<<24)>>>0;const length=p[5];if(length>8||p.length<9+length)return null;return{timestamp,pid:p[4],id:p[4]&63,length,side:p[6]?'target':'controller',parityOk:!!(p[7]&1),checksumOk:!!(p[7]&2),collision:!!(p[7]&4),data:p.slice(8,8+length),checksum:p[8+length]};}
export function allowMask(ids){const out=new Uint8Array(8);for(const id of ids)if(id>=0&&id<64)out[id>>3]|=1<<(id&7);return out;}
export function txPayload(id,data,side=0,checksum=1){return new Uint8Array([id&63,data.length,checksum,side,...data]);}
