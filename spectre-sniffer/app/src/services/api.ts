//==============================================================================
// api.ts — Spectre-Sniffer REST API Client
// Author: jayis1
// Description: HTTP client for communicating with the Spectre-Sniffer device
//              over WiFi. Provides typed methods for all API endpoints.
//==============================================================================

import axios, { AxiosInstance, AxiosResponse } from 'axios';
import {
  DeviceStatus,
  SpectrumData,
  TempestFrame,
  CryptoAnalysisResult,
  CaptureFile,
  CaptureConfig,
  DeviceConfig,
  ProbeCalibration,
} from '../types';

const DEFAULT_BASE_URL = 'http://192.168.4.1';
const REQUEST_TIMEOUT_MS = 10000;

//==============================================================================
// API Client Class
//==============================================================================
class SpectreAPIClient {
  private client: AxiosInstance;
  private baseUrl: string;

  constructor(baseUrl: string = DEFAULT_BASE_URL) {
    this.baseUrl = baseUrl;
    this.client = axios.create({
      baseURL: baseUrl,
      timeout: REQUEST_TIMEOUT_MS,
      headers: {
        'Content-Type': 'application/json',
        'User-Agent': 'SpectreSniffer-App/1.0',
      },
    });
  }

  /**
   * Update the base URL (e.g., when device IP changes)
   */
  setBaseUrl(url: string): void {
    this.baseUrl = url;
    this.client.defaults.baseURL = url;
  }

  /**
   * Get the current base URL
   */
  getBaseUrl(): string {
    return this.baseUrl;
  }

  //============================================================================
  // Device Status
  //============================================================================

  /**
   * Get device status and telemetry
   */
  async getStatus(): Promise<DeviceStatus> {
    const response = await this.client.get<DeviceStatus>('/api/status');
    return response.data;
  }

  //============================================================================
  // Capture Management
  //============================================================================

  /**
   * Start an IQ capture with specified parameters
   */
  async startCapture(config: CaptureConfig): Promise<{ captureId: string }> {
    const response = await this.client.post('/api/capture/start', config);
    return response.data;
  }

  /**
   * Stop the currently active capture
   */
  async stopCapture(): Promise<void> {
    await this.client.post('/api/capture/stop');
  }

  /**
   * List all captures on the SD card
   */
  async listCaptures(): Promise<CaptureFile[]> {
    const response = await this.client.get<CaptureFile[]>('/api/capture/list');
    return response.data;
  }

  /**
   * Download a capture file
   */
  async downloadCapture(captureId: string): Promise<ArrayBuffer> {
    const response = await this.client.get(`/api/capture/download/${captureId}`, {
      responseType: 'arraybuffer',
    });
    return response.data;
  }

  /**
   * Delete a capture file
   */
  async deleteCapture(captureId: string): Promise<void> {
    await this.client.delete(`/api/capture/${captureId}`);
  }

  //============================================================================
  // Spectrum Analysis
  //============================================================================

  /**
   * Get current FFT spectrum data
   */
  async getSpectrum(): Promise<SpectrumData> {
    const response = await this.client.get<SpectrumData>('/api/spectrum');
    return response.data;
  }

  //============================================================================
  // Tempest Decoding
  //============================================================================

  /**
   * Start Tempest decoding with specified pixel clock
   */
  async startTempest(pixelClockHz: number): Promise<void> {
    await this.client.post('/api/tempest/start', { pixelClockHz });
  }

  /**
   * Stop Tempest decoding
   */
  async stopTempest(): Promise<void> {
    await this.client.post('/api/tempest/stop');
  }

  /**
   * Get the latest reconstructed Tempest frame
   */
  async getTempestImage(): Promise<TempestFrame> {
    const response = await this.client.get<TempestFrame>('/api/tempest/image');
    return response.data;
  }

  //============================================================================
  // Crypto Analysis
  //============================================================================

  /**
   * Get captured crypto traces
   */
  async getCryptoTraces(): Promise<{ traces: number[][]; plaintexts: number[][] }> {
    const response = await this.client.get('/api/crypto/traces');
    return response.data;
  }

  /**
   * Run DPA/CPA analysis on captured traces
   */
  async runCryptoAnalysis(params: {
    algorithm: string;
    numTraces: number;
    analysisType: 'DPA' | 'CPA';
    knownPlaintext?: number[];
  }): Promise<CryptoAnalysisResult> {
    const response = await this.client.post<CryptoAnalysisResult>(
      '/api/crypto/analyze',
      params
    );
    return response.data;
  }

  /**
   * Get current crypto analysis progress
   */
  async getCryptoProgress(): Promise<CryptoAnalysisResult> {
    const response = await this.client.get<CryptoAnalysisResult>(
      '/api/crypto/progress'
    );
    return response.data;
  }

  //============================================================================
  // Configuration
  //============================================================================

  /**
   * Get device configuration
   */
  async getConfig(): Promise<DeviceConfig> {
    const response = await this.client.get<DeviceConfig>('/api/config');
    return response.data;
  }

  /**
   * Update device configuration
   */
  async updateConfig(config: Partial<DeviceConfig>): Promise<DeviceConfig> {
    const response = await this.client.put<DeviceConfig>('/api/config', config);
    return response.data;
  }

  //============================================================================
  // Calibration
  //============================================================================

  /**
   * Run probe calibration
   */
  async calibrateProbe(probeType: string): Promise<ProbeCalibration> {
    const response = await this.client.post<ProbeCalibration>('/api/calibrate', {
      probeType,
    });
    return response.data;
  }

  /**
   * Get current calibration data
   */
  async getCalibration(): Promise<ProbeCalibration> {
    const response = await this.client.get<ProbeCalibration>('/api/calibrate');
    return response.data;
  }

  //============================================================================
  // Device Control
  //============================================================================

  /**
   * Reboot the device
   */
  async reboot(): Promise<void> {
    await this.client.post('/api/reboot');
  }

  /**
   * Put device into deep sleep
   */
  async sleep(): Promise<void> {
    await this.client.post('/api/sleep');
  }

  /**
   * Scan for WiFi networks
   */
  async wifiScan(): Promise<{ ssid: string; rssi: number; channel: number }[]> {
    const response = await this.client.get('/api/wifi/scan');
    return response.data;
  }

  /**
   * Connect to a WiFi network
   */
  async wifiConnect(ssid: string, password: string): Promise<void> {
    await this.client.post('/api/wifi/connect', { ssid, password });
  }

  /**
   * Get device logs
   */
  async getLogs(lines: number = 100): Promise<string[]> {
    const response = await this.client.get<string[]>('/api/logs', {
      params: { lines },
    });
    return response.data;
  }
}

//==============================================================================
// Singleton Export
//==============================================================================
export const SpectreAPI = new SpectreAPIClient();
export default SpectreAPIClient;
