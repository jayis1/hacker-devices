/**
 * utils/pcapng.js — assemble a .pcapng file from 1553cap records
 *
 * Author: jayis1
 * License: MIT
 *
 * The firmware streams captures in three formats. This helper turns the
 * compact CSV form into a pcapng blob a host can open in Wireshark with
 * a 1553 dissector (DLT 211, LINKTYPE_MIL1553).
 *
 * pcapng block layout we emit:
 *   SHB (Section Header Block)
 *   IDB (Interface Description Block, linktype=211)
 *   EPB (Enhanced Packet Block) per captured 1553 word
 */

/* ---- pcapng block types ---- */
const SHB_TYPE = 0x0a0d0d0a;
const IDB_TYPE = 0x00000001;
const EPB_TYPE = 0x00000006;

/* ---- little-endian helpers ---- */
function u32(v) {
  return [
    v & 0xff, (v >> 8) & 0xff, (v >> 16) & 0xff, (v >> 24) & 0xff,
  ];
}
function u16(v) { return [v & 0xff, (v >> 8) & 0xff]; }
function pad4(n) { return (4 - (n % 4)) % 4; }

function block(type, body) {
  const len = 12 + body.length + pad4(body.length);
  const totalLen = len + 4; /* includes trailing total-length field */
  const hdr = [...u32(type), ...u32(totalLen, 0)];
  const pad = new Array(pad4(body.length)).fill(0);
  const trail = [...u32(totalLen, 0)];
  return [...hdr, ...body, ...pad, ...trail];
}

export function buildSectionHeader() {
  /* byte order magic + version 1.0 + section length -1 (unknown) */
  const bom = [0x1a, 0x2b, 0x3c, 0x4d];
  const ver = [...u16(1), ...u16(0)];
  const secLen = [...u32(0xffffffff)];
  return block(SHB_TYPE, [...bom, ...ver, ...secLen]);
}

export function buildInterfaceDesc() {
  /* linktype 211 (LINKTYPE_MIL1553), snaplen 65535 */
  const linkType = [...u16(211)];
  const reserved = [0, 0];
  const snapLen = [...u32(65535)];
  return block(IDB_TYPE, [...linkType, ...reserved, ...snapLen]);
}

export function buildPacket(tsUs, data) {
  /* tsHi, tsLo (us since epoch), caplen, origlen, then data */
  const tsHi = [...u32(Math.floor(tsUs / 1e6))];
  const tsLo = [...u32(tsUs % 1e6)];
  const caplen = [...u32(data.length)];
  const origlen = [...u32(data.length)];
  return block(EPB_TYPE, [...tsHi, ...tsLo, ...caplen, ...origlen, ...data]);
}

/**
 * Convert an array of CSV capture lines (as emitted by `bm flush`) into
 * a Uint8Array holding a valid pcapng file.
 *
 * Each CSV line: ts,ch,type,word20,status
 */
export function csvToPcapng(csvLines) {
  const bytes = [];
  bytes.push(...buildSectionHeader());
  bytes.push(...buildInterfaceDesc());
  for (const line of csvLines) {
    const parts = line.split(',');
    if (parts.length < 4) continue;
    const ts = parseInt(parts[0], 10) || 0;
    const ch = parseInt(parts[1], 10) & 1;
    const type = parts[2];
    const word = parseInt(parts[3], 16) & 0xfffff;
    /* build a tiny synthetic 1553 frame: [ch][type][word(3 bytes BE)] */
    const data = [ch, type.charCodeAt(0),
      (word >> 16) & 0xff, (word >> 8) & 0xff, word & 0xff];
    bytes.push(...buildPacket(ts, data));
  }
  return new Uint8Array(bytes);
}

/* ---- Save a file via the host's sharing sheet ---- */
export async function savePcapng(csvLines, filename) {
  const { default: FileSystem } = await import('expo-file-system');
  const { default: Sharing } = await import('expo-sharing');
  const bytes = csvToPcapng(csvLines);
  /* Convert to base64 for FileSystem.writeAsStringAsync */
  let b64 = '';
  for (let i = 0; i < bytes.length; i++) {
    b64 += String.fromCharCode(bytes[i]);
  }
  const b64encoded = btoa(b64);
  const path = FileSystem.documentDirectory + filename;
  await FileSystem.writeAsStringAsync(path, b64encoded, {
    encoding: FileSystem.EncodingType.Base64,
  });
  await Sharing.shareAsync(path, { mimeType: 'application/vnd.tcpdump.pcap' });
}