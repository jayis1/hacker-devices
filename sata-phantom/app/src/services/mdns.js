/**
 * mdns.js — mDNS Service Discovery for SATA Phantom
 * Author: jayis1
 */

const MOCK_DEVICES = [
  {
    id: 'sp-001',
    name: 'SATA Phantom #1',
    ip: '192.168.4.1',
    port: 8080,
    mac: '02:DE:AD:BE:EF:01',
    mode: 0,
    linkUp: false,
    speed: 0,
    battery: 3850,
    framesCaptured: 1245,
    rulesActive: 3,
    firmware: '1.0.0',
  },
];

/**
 * Discover SATA Phantom devices on the local network.
 * Uses: (1) mDNS query for _sata-phantom._tcp.local
 *        (2) BLE scan for SATA Phantom service UUID
 *        (3) Falls back to mock data for testing
 *
 * In production, this uses react-native-tcp-socket + dns-sd or
 * react-native-ble-plx for BLE scanning.
 */
export const discoverDevices = async () => {
  // TODO: Real mDNS implementation using native DNS-SD APIs
  // For now, return mock data after a brief scan delay
  return new Promise((resolve) => {
    setTimeout(() => {
      resolve(MOCK_DEVICES);
    }, 1500);
  });
};

/**
 * Connect to a SATA Phantom device and set the API base URL.
 */
export const connectToDevice = async (device) => {
  const { setBaseUrl } = require('./api');
  setBaseUrl(`http://${device.ip}:${device.port}`);
  return { success: true, device };
};
