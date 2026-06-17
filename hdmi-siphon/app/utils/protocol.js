/**
 * protocol.js — HDMI-Siphon Wire Protocol Client
 * Author: jayis1
 * Version: 1.0.0
 * License: Proprietary — Authorized Security Research Use Only
 *
 * Implements the client-side wire protocol for communicating with
 * the HDMI-Siphon device over WebSocket (WiFi) or BLE GATT.
 *
 * JSON command format:  {"cmd":"<command>","params":{...}}
 * JSON response format: {"status":"ok|error",<key>:<value>,...}
 */

class ProtocolClient {
  /**
   * @param {Object} options - Client configuration
   * @param {Function} options.onStatus - Status update callback
   * @param {Function} options.onDisconnect - Disconnect callback
   * @param {Function} options.onError - Error callback
   * @param {string} options.host - Device IP/hostname (default: 192.168.4.1)
   * @param {number} options.port - WebSocket port (default: 8080)
   */
  constructor(options = {}) {
    this.onStatus = options.onStatus || (() => {});
    this.onDisconnect = options.onDisconnect || (() => {});
    this.onError = options.onError || (() => {});
    this.host = options.host || '192.168.4.1';
    this.port = options.port || 8080;
    this.ws = null;
    this.connected = false;
    this.reconnectTimer = null;
    this.reconnectInterval = 5000; // 5 seconds
    this.requestId = 0;
    this.pendingRequests = new Map();
  }

  /**
   * Connect to the HDMI-Siphon device via WebSocket
   */
  connect() {
    if (this.ws) {
      this.disconnect();
    }

    const url = `ws://${this.host}:${this.port}`;
    console.log(`[Protocol] Connecting to ${url}...`);

    try {
      this.ws = new WebSocket(url);

      this.ws.onopen = () => {
        console.log('[Protocol] Connected');
        this.connected = true;
        // Request initial status
        this.sendCommand('status');
      };

      this.ws.onmessage = event => {
        try {
          const data = JSON.parse(event.data);
          this._handleMessage(data);
        } catch (err) {
          console.warn('[Protocol] Invalid JSON:', event.data);
        }
      };

      this.ws.onclose = () => {
        console.log('[Protocol] Disconnected');
        this.connected = false;
        this.onDisconnect();
        this._scheduleReconnect();
      };

      this.ws.onerror = error => {
        console.warn('[Protocol] Error:', error.message);
        this.onError(error);
      };
    } catch (err) {
      console.error('[Protocol] Connection failed:', err);
      this.onError(err);
      this._scheduleReconnect();
    }
  }

  /**
   * Disconnect from the device
   */
  disconnect() {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    this.connected = false;
  }

  /**
   * Send a command to the device
   * @param {string} cmd - Command name
   * @param {Object} params - Command parameters (optional)
   * @param {Function} callback - Response callback (optional)
   */
  sendCommand(cmd, params = {}, callback = null) {
    if (!this.connected || !this.ws) {
      console.warn('[Protocol] Cannot send command — not connected');
      if (callback) {
        callback({status: 'error', msg: 'not connected'});
      }
      return;
    }

    const requestId = ++this.requestId;
    const message = JSON.stringify({
      id: requestId,
      cmd: cmd,
      params: params,
    });

    if (callback) {
      this.pendingRequests.set(requestId, callback);
    }

    try {
      this.ws.send(message);
      console.log(`[Protocol] Sent: ${cmd} (id=${requestId})`);
    } catch (err) {
      console.error('[Protocol] Send failed:', err);
      if (callback) {
        this.pendingRequests.delete(requestId);
        callback({status: 'error', msg: err.message});
      }
    }

    // Auto-clear callback after timeout
    if (callback) {
      setTimeout(() => {
        if (this.pendingRequests.has(requestId)) {
          this.pendingRequests.delete(requestId);
          callback({status: 'error', msg: 'timeout'});
        }
      }, 10000);
    }
  }

  /**
   * Trigger a single frame capture
   * @param {Function} callback - Called with {status, frame_id}
   */
  captureFrame(callback) {
    this.sendCommand('capture', {}, callback);
  }

  /**
   * Get device status
   * @param {Function} callback
   */
  getStatus(callback) {
    this.sendCommand('status', {}, callback);
  }

  /**
   * Set device operating mode
   * @param {string} mode - 'passthrough' | 'invert' | 'grayscale' | 'blank'
   * @param {Function} callback
   */
  setMode(mode, callback) {
    this.sendCommand('mode', {mode: mode}, callback);
  }

  /**
   * Inject custom EDID data
   * @param {string} edidHex - Hex-encoded EDID binary (256 hex chars)
   * @param {Function} callback
   */
  setEDID(edidHex, callback) {
    this.sendCommand('edid_set', {edid_hex: edidHex}, callback);
  }

  /**
   * Disable HDCP support in EDID
   * @param {Function} callback
   */
  disableHDCP(callback) {
    this.sendCommand('edid_set', {edid_hex: ''}, callback); // Uses default no-HDCP
  }

  /**
   * Send CEC command
   * @param {number} address - Destination logical address (0-15)
   * @param {number} opcode - CEC opcode
   * @param {number[]} payload - Optional payload bytes
   * @param {Function} callback
   */
  sendCEC(address, opcode, payload = [], callback) {
    this.sendCommand('cec_send', {
      address: address,
      opcode: opcode,
      payload: payload,
    }, callback);
  }

  /**
   * Inject OSD text overlay
   * @param {string} text - Text to display
   * @param {number} x - X position in pixels
   * @param {number} y - Y position in pixels
   * @param {string} color - Hex color (e.g., "#FF0000")
   * @param {number} alpha - Alpha blend (0-255)
   * @param {Function} callback
   */
  setOSD(text, x = 100, y = 100, color = '#FF0000', alpha = 200, callback) {
    this.sendCommand('osd_text', {
      text: text,
      x: x,
      y: y,
      color: color,
      alpha: alpha,
    }, callback);
  }

  /**
   * Start covert recording mode
   * @param {Object} config - {interval_ms, max_frames, trigger}
   * @param {Function} callback
   */
  startCovert(config = {}, callback) {
    this.sendCommand('covert_start', {
      interval_ms: config.interval_ms || 1000,
      max_frames: config.max_frames || 100,
      trigger: config.trigger || 'motion',
    }, callback);
  }

  /**
   * Stop covert recording mode
   * @param {Function} callback
   */
  stopCovert(callback) {
    this.sendCommand('covert_stop', {}, callback);
  }

  /**
   * Set device configuration
   * @param {Object} config - Configuration object
   * @param {Function} callback
   */
  setConfig(config, callback) {
    this.sendCommand('config', config, callback);
  }

  /**
   * Handle incoming message from device
   * @private
   */
  _handleMessage(data) {
    // Check if this is a response to a pending request
    if (data.id && this.pendingRequests.has(data.id)) {
      const callback = this.pendingRequests.get(data.id);
      this.pendingRequests.delete(data.id);
      callback(data);
      return;
    }

    // Check if this is a status update (no id field, has status field)
    if (data.status === 'ok' && data.device) {
      this.onStatus(data);
      return;
    }

    // Generic response
    if (data.status) {
      this.onStatus(data);
    }
  }

  /**
   * Schedule reconnection attempt
   * @private
   */
  _scheduleReconnect() {
    if (this.reconnectTimer) return;
    this.reconnectTimer = setTimeout(() => {
      this.reconnectTimer = null;
      console.log('[Protocol] Attempting reconnection...');
      this.connect();
    }, this.reconnectInterval);
  }
}

export {ProtocolClient};
