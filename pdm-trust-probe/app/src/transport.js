// WebUSB and deterministic mock transport for PDM Trust Probe
// Author: jayis1
export class ProbeTransport {
  constructor(mock = false) { this.mock = mock; this.device = null; this.history = []; }
  async connect() {
    if (this.mock) return { productName: 'PDM Trust Probe Mock', mock: true };
    if (!globalThis.navigator?.usb) throw new Error('WebUSB is unavailable; use mock mode or a supported browser');
    this.device = await navigator.usb.requestDevice({ filters: [] });
    await this.device.open();
    if (!this.device.configuration) await this.device.selectConfiguration(1);
    await this.device.claimInterface(0);
    return { productName: this.device.productName || 'PDM Trust Probe', mock: false };
  }
  async send(frame) {
    if (!(frame instanceof Uint8Array)) throw new TypeError('frame');
    this.history.push(frame.slice());
    if (this.mock) return { status: 0, data: new Uint8Array([0]) };
    if (!this.device?.opened) throw new Error('device disconnected');
    const result = await this.device.transferOut(1, frame);
    if (result.status !== 'ok') throw new Error(`USB transfer failed: ${result.status}`);
    const response = await this.device.transferIn(1, 64);
    if (response.status !== 'ok' || !response.data || response.data.byteLength < 1) throw new Error('Missing device acknowledgement');
    const status = response.data.getUint8(0);
    if (status !== 0) throw new Error(`Device rejected command with status ${status}`);
    return { status, data: new Uint8Array(response.data.buffer, response.data.byteOffset, response.data.byteLength) };
  }
  async close() { if (this.device?.opened) await this.device.close(); this.device = null; }
}
