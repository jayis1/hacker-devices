/**
 * cableTypes.js — cable type name lookup for the app
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Must match the cable_type_t enum in firmware/drivers/tdr_engine.h
 */

export const CABLE_TYPE_NAMES = [
  'Unknown',                    // 0
  'Cat 5e UTP 100Ω',            // 1
  'Cat 6 UTP 100Ω',             // 2
  'Cat 6A UTP 100Ω',            // 3
  'Cat 7 S/FTP 100Ω',           // 4
  'Profibus DP 150Ω',           // 5
  'RS-485 Belden 9841 120Ω',    // 6
  'MIL-1553 Twinax 78Ω',        // 7
  'POTS Riser 100Ω',            // 8
  'Power/Mains (REFUSE)',       // 9
];

export const CABLE_TYPE_COLORS = [
  '#6e7681',  // Unknown
  '#00d4aa',  // Cat 5e
  '#00d4aa',  // Cat 6
  '#00d4aa',  // Cat 6A
  '#a371f7',  // Cat 7 (shielded)
  '#d29922',  // Profibus
  '#ff6b35',  // RS-485
  '#f85149',  // MIL-1553
  '#8b949e',  // POTS
  '#f85149',  // Power
];