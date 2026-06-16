/**
 * websocket.js — WebSocket Stream Handler
 * Author: jayis1
 */

class SATAWebSocket {
  constructor() {
    this.ws = null;
    this.reconnectTimer = null;
    this.listeners = {};
  }

  connect(url) {
    if (this.ws) this.disconnect();

    this.ws = new WebSocket(url);
    this.ws.onopen = () => {
      console.log('WebSocket connected');
      this.emit('connected');
    };
    this.ws.onclose = () => {
      console.log('WebSocket disconnected');
      this.emit('disconnected');
      // Auto-reconnect after 5s
      this.reconnectTimer = setTimeout(() => this.connect(url), 5000);
    };
    this.ws.onerror = (e) => {
      console.error('WebSocket error:', e.message);
      this.emit('error', e);
    };
    this.ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        this.emit('data', data);
      } catch {
        this.emit('raw', event.data);
      }
    };
  }

  disconnect() {
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  on(event, callback) {
    if (!this.listeners[event]) this.listeners[event] = [];
    this.listeners[event].push(callback);
  }

  off(event, callback) {
    if (!this.listeners[event]) return;
    this.listeners[event] = this.listeners[event].filter(cb => cb !== callback);
  }

  emit(event, data) {
    (this.listeners[event] || []).forEach(cb => cb(data));
  }

  send(data) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data));
    }
  }
}

export default new SATAWebSocket();
