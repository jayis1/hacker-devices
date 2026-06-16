//==============================================================================
// types.ts — Spectre-Sniffer Type Definitions
// Author: jayis1
// Description: TypeScript type definitions for the Spectre-Sniffer companion app.
//==============================================================================

export interface DeviceStatus {
  /** Device is connected and responding */
  connected: boolean;
  /** Current operating mode */
  mode: OperatingMode;
  /** Battery level percentage (0-100) */
  batteryPercent: number;
  /** Battery voltage in millivolts */
  batteryMillivolts: number;
  /** FPGA die temperature in Celsius */
  fpgaTemperature: number;
  /** System uptime in seconds */
  uptimeSeconds: number;
  /** Current center frequency in Hz */
  centerFrequencyHz: number;
  /** Current bandwidth in Hz */
  bandwidthHz: number;
  /** LNA gain in dB */
  lnaGainDb: number;
  /** Whether a capture is currently active */
  captureActive: boolean;
  /** Number of samples captured so far */
  samplesCaptured: number;
  /** SD card free space in bytes */
  sdFreeBytes: number;
  /** SD card total space in bytes */
  sdTotalBytes: number;
  /** WiFi signal strength in dBm */
  wifiSignalDbm: number;
  /** Number of connected clients */
  wifiClients: number;
}

export enum OperatingMode {
  IDLE = 0,
  SPECTRUM_ANALYZER = 1,
  IQ_CAPTURE = 2,
  TEMPEST_DECODE = 3,
  CRYPTO_ANALYSIS = 4,
  KEYSTROKE_ANALYSIS = 5,
  SIGNAL_GENERATOR = 6,
  CALIBRATION = 7,
  SETTINGS = 8,
  FILE_BROWSER = 9,
  SLEEP = 10,
}

export const OperatingModeNames: Record<OperatingMode, string> = {
  [OperatingMode.IDLE]: 'Idle',
  [OperatingMode.SPECTRUM_ANALYZER]: 'Spectrum Analyzer',
  [OperatingMode.IQ_CAPTURE]: 'IQ Capture',
  [OperatingMode.TEMPEST_DECODE]: 'Tempest Decode',
  [OperatingMode.CRYPTO_ANALYSIS]: 'Crypto Analysis',
  [OperatingMode.KEYSTROKE_ANALYSIS]: 'Keystroke Analysis',
  [OperatingMode.SIGNAL_GENERATOR]: 'Signal Generator',
  [OperatingMode.CALIBRATION]: 'Calibration',
  [OperatingMode.SETTINGS]: 'Settings',
  [OperatingMode.FILE_BROWSER]: 'File Browser',
  [OperatingMode.SLEEP]: 'Sleep',
};

export interface SpectrumData {
  /** Array of FFT magnitude values (length = FFT_BINS/2) */
  magnitudes: number[];
  /** Center frequency in Hz */
  centerFrequencyHz: number;
  /** Span in Hz */
  spanHz: number;
  /** Maximum magnitude value in this frame */
  maxMagnitude: number;
  /** Timestamp of capture */
  timestamp: number;
}

export interface WaterfallData {
  /** Array of FFT frames, each frame is an array of magnitudes */
  frames: number[][];
  /** Number of bins per frame */
  binsPerFrame: number;
  /** Center frequency in Hz */
  centerFrequencyHz: number;
  /** Span in Hz */
  spanHz: number;
}

export interface TempestFrame {
  /** Width of reconstructed image in pixels */
  width: number;
  /** Height of reconstructed image in pixels */
  height: number;
  /** Pixel data as RGBA byte array */
  pixelData: number[];
  /** Signal quality estimate (0.0 - 1.0) */
  signalQuality: number;
  /** Detected video mode name */
  videoMode: string;
  /** Frame number */
  frameNumber: number;
}

export interface CryptoAnalysisResult {
  /** Recovered key bytes (16 for AES-128) */
  keyBytes: number[];
  /** Confidence score per key byte (0.0 - 1.0) */
  confidence: number[];
  /** Number of traces used */
  numTraces: number;
  /** Analysis type (DPA or CPA) */
  analysisType: 'DPA' | 'CPA';
  /** Target algorithm name */
  algorithm: string;
  /** Progress of analysis (0.0 - 1.0) */
  progress: number;
  /** Whether analysis is still running */
  running: boolean;
}

export interface CaptureFile {
  /** File name */
  name: string;
  /** File size in bytes */
  sizeBytes: number;
  /** Capture timestamp (Unix epoch) */
  timestamp: number;
  /** Center frequency in Hz */
  centerFrequencyHz: number;
  /** Sample rate in Hz */
  sampleRateHz: number;
  /** Duration in milliseconds */
  durationMs: number;
}

export interface CaptureConfig {
  centerFrequencyHz: number;
  bandwidthHz: number;
  durationMs: number;
  lnaGainDb: number;
  useDownconverter: boolean;
  enableTrigger: boolean;
  triggerLevel: number;
  filename: string;
}

export interface DeviceConfig {
  /** WiFi AP SSID */
  wifiApSsid: string;
  /** WiFi AP password */
  wifiApPassword: string;
  /** WiFi channel */
  wifiChannel: number;
  /** BLE advertisement interval in ms */
  bleAdvertiseInterval: number;
  /** Display brightness (0-255) */
  displayBrightness: number;
  /** Auto power-off timeout in seconds (0 = disabled) */
  autoPowerOffSeconds: number;
  /** Calibration data reference */
  calibrationProfile: string;
  /** Date format for file naming */
  dateFormat: string;
}

export interface ProbeCalibration {
  /** Probe type */
  probeType: 'H_FIELD' | 'E_FIELD' | 'LOG_PERIODIC' | 'ACTIVE_DIFFERENTIAL';
  /** Frequency range start in Hz */
  freqStartHz: number;
  /** Frequency range end in Hz */
  freqEndHz: number;
  /** Gain correction values per frequency point */
  gainCorrection: number[];
  /** Calibration date */
  calibrationDate: string;
}
