/**
 * api.js — REST API Client for SATA Phantom
 * Author: jayis1
 */

const SATA_PHANTOM_PORT = 8080;

let baseUrl = 'http://192.168.4.1:' + SATA_PHANTOM_PORT;
// In production, this is set by DiscoveryScreen when connecting

export const setBaseUrl = (url) => {
  baseUrl = url;
};

const apiRequest = async (endpoint, options = {}) => {
  const url = `${baseUrl}${endpoint}`;
  const response = await fetch(url, {
    headers: { 'Content-Type': 'application/json', ...options.headers },
    ...options,
  });
  return response.json();
};

export const getStatus = async () => {
  try {
    const data = await apiRequest('/api/v1/status');
    return {
      mode: data.mode || 0,
      linkUp: !!data.link_up,
      speed: data.speed || 0,
      battery: data.battery_mv || 3700,
      framesRead: data.frames_read || 0,
      framesWrite: data.frames_write || 0,
      rulesActive: data.rules_active || 0,
      framesCaptured: data.frames_captured || 0,
      bytesExfiltrated: data.bytes_exfiltrated || 0,
      uptime: data.uptime || 0,
      firmware: data.firmware || '1.0.0',
      temperature: data.temperature || 0,
    };
  } catch (e) {
    console.warn('Status fetch error:', e.message);
    return {
      mode: 0, linkUp: false, speed: 0, battery: 0,
      framesRead: 0, framesWrite: 0, rulesActive: 0,
      framesCaptured: 0, bytesExfiltrated: 0, uptime: 0,
    };
  }
};

export const getTrafficStats = async () => {
  try {
    const data = await apiRequest('/api/v1/traffic');
    return {
      timestamp: data.timestamp || Date.now(),
      readsPerSec: data.reads_per_sec || 0,
      writesPerSec: data.writes_per_sec || 0,
    };
  } catch {
    return { timestamp: Date.now(), readsPerSec: 0, writesPerSec: 0 };
  }
};

export const getRules = async () => {
  try {
    const data = await apiRequest('/api/v1/rules');
    return data.rules || [];
  } catch {
    return [];
  }
};

export const addRule = async (rule) => {
  return apiRequest('/api/v1/rules', {
    method: 'POST',
    body: JSON.stringify(rule),
  });
};

export const deleteRule = async (index) => {
  return apiRequest(`/api/v1/rules/${index}`, { method: 'DELETE' });
};

export const setMode = async (mode) => {
  return apiRequest('/api/v1/mode', {
    method: 'POST',
    body: JSON.stringify({ mode }),
  });
};

export const injectFrame = async (data, fisType) => {
  return apiRequest('/api/v1/inject', {
    method: 'POST',
    body: JSON.stringify({ data, fis_type: fisType }),
  });
};

export const getExfilList = async () => {
  try {
    const data = await apiRequest('/api/v1/exfil/list');
    return data.files || [];
  } catch {
    return [];
  }
};

export const downloadExfil = async (id) => {
  return apiRequest(`/api/v1/exfil/download/${id}`);
};

export const getConfig = async () => {
  try {
    return await apiRequest('/api/v1/config');
  } catch {
    return {};
  }
};

export const updateConfig = async (config) => {
  return apiRequest('/api/v1/config', {
    method: 'PUT',
    body: JSON.stringify(config),
  });
};

export const sendConsoleCommand = async (command) => {
  return apiRequest('/api/v1/console', {
    method: 'POST',
    body: JSON.stringify({ command }),
  });
};

export default {
  setBaseUrl, getStatus, getTrafficStats, getRules, addRule, deleteRule,
  setMode, injectFrame, getExfilList, downloadExfil, getConfig, updateConfig,
  sendConsoleCommand,
};
