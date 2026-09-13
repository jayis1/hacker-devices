// Web Serial transport with bounded framing. Author: jayis1. SPDX-License-Identifier: MIT
import { frame } from './protocol.js';

export class CartographerTransport {
  constructor(onEvent) {
    this.port = null;
    this.reader = null;
    this.writer = null;
    this.sequence = 1;
    this.onEvent = onEvent;
    this.running = false;
    this.pending = new Uint8Array();
  }

  async connect() {
    if (!navigator.serial) throw new Error('Web Serial requires Chromium over HTTPS or localhost');
    this.port = await navigator.serial.requestPort({ filters: [{ usbVendorId: 0x1209, usbProductId: 0x4f57 }] });
    await this.port.open({ baudRate: 115200, bufferSize: 4096 });
    this.writer = this.port.writable.getWriter();
    this.running = true;
    this.readLoop();
  }

  async send(command, payload = new Uint8Array()) {
    if (!this.writer) throw new Error('device not connected');
    await this.writer.write(frame(command, payload, this.sequence++));
  }

  async readLoop() {
    this.reader = this.port.readable.getReader();
    try {
      while (this.running) {
        const { value, done } = await this.reader.read();
        if (done) break;
        const merged = new Uint8Array(this.pending.length + value.length);
        merged.set(this.pending); merged.set(value, this.pending.length);
        let offset = 0;
        while (merged.length - offset >= 16) {
          this.onEvent(merged.slice(offset, offset + 16));
          offset += 16;
        }
        this.pending = merged.slice(offset);
      }
    } finally {
      this.reader.releaseLock();
      this.reader = null;
    }
  }

  async disconnect() {
    this.running = false;
    if (this.reader) await this.reader.cancel();
    if (this.writer) { this.writer.releaseLock(); this.writer = null; }
    if (this.port) { await this.port.close(); this.port = null; }
  }
}
