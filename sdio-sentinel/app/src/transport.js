// SDIO Sentinel device and demo transports. Author: jayis1.
import { Modes, commandName } from './protocol.js';

const sleep = (milliseconds) => new Promise((resolve) => setTimeout(resolve, milliseconds));

export class DemoTransport {
  constructor() {
    this.connected = false;
    this.listener = null;
    this.timer = null;
    this.sequence = 0;
    this.mode = Modes.SAFE_BYPASS;
    this.started = Date.now();
  }

  async connect() {
    await sleep(180);
    this.connected = true;
    this.timer = setInterval(() => this.emitFrame(), 1200);
    return { productName: 'SDIO Sentinel Demo', serialNumber: 'DEMO-JAYIS1' };
  }

  async disconnect() {
    this.connected = false;
    clearInterval(this.timer);
    this.timer = null;
  }

  subscribe(listener) {
    this.listener = listener;
    return () => { this.listener = null; };
  }

  async status() {
    if (!this.connected) throw new Error('Demo device is offline');
    return {
      boardId: 0x53445331,
      uptimeMs: Date.now() - this.started,
      framesSeen: this.sequence,
      blocked: this.mode === Modes.ENFORCE_POLICY ? 2 : 0,
      anomalies: Math.floor(this.sequence / 4),
      clockHz: 25_000_000,
      mode: this.mode,
      cardState: 5,
      armed: false,
      cardPresent: true,
    };
  }

  async setMode(mode, scopeConfirmed = false) {
    if (mode > Modes.PASSIVE_OBSERVE && !scopeConfirmed) {
      throw new Error('Active modes require authorization confirmation and physical arming');
    }
    await sleep(120);
    this.mode = mode;
    return this.status();
  }

  async addRule(rule) {
    await sleep(90);
    return { ...rule, id: crypto.randomUUID(), enabled: true };
  }

  emitFrame() {
    if (!this.listener || !this.connected) return;
    const commands = [17, 18, 13, 24, 52, 17, 42, 53];
    const command = commands[this.sequence % commands.length];
    const dangerous = [24, 42, 53].includes(command);
    const frame = {
      sequence: this.sequence,
      timeMs: Date.now() - this.started,
      direction: 'host-to-card',
      command,
      name: commandName(command),
      argument: command === 24 ? 2 : command === 53 ? 0x80002001 : 8192,
      severity: dangerous ? 'warning' : 'info',
      score: dangerous ? 48 : 0,
      action: dangerous && this.mode === Modes.ENFORCE_POLICY ? 'blocked' : 'allowed',
      summary: dangerous ? 'Transaction intersects a protected policy boundary' : 'Normal transaction',
    };
    this.sequence += 1;
    this.listener(frame);
  }
}

export class WebSerialTransport {
  constructor() {
    this.port = null;
    this.reader = null;
  }

  static supported() {
    return typeof navigator !== 'undefined' && 'serial' in navigator;
  }

  async connect() {
    if (!WebSerialTransport.supported()) throw new Error('Web Serial is unavailable');
    this.port = await navigator.serial.requestPort({ filters: [{ usbVendorId: 0x0483 }] });
    await this.port.open({ baudRate: 115200, bufferSize: 4096 });
    return this.port.getInfo();
  }

  async disconnect() {
    if (this.reader) {
      await this.reader.cancel();
      this.reader.releaseLock();
      this.reader = null;
    }
    if (this.port) await this.port.close();
    this.port = null;
  }
}
