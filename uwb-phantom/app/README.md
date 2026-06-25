# UWB-PHANTOM Companion App

**Author:** jayis1 · **License:** GPL-3.0-or-later

React Native + Expo companion app for the UWB-PHANTOM hardware.

## Build

```bash
npm install
npx expo start
```

Android: `npx expo start --android` · iOS: `npx expo start --ios`

## Screens

| Screen | Purpose |
|---|---|
| Connect | BLE scan + pair |
| Dashboard | Live status (battery, mode, distance, RSSI) |
| TWR | Two-way ranging control + live distance chart |
| Sniffer | Passive capture, channel/duration, frame list |
| STS Audit | Run STS enforcement suites against a target verifier |
| Relay | Relay engine control (transparent / shrink / jitter) |
| Tracker | Detect UWB emitters following the operator |
| Targets | Authorised-target list (HMAC-signed) |
| Settings | Firmware info, antenna delay, SD format, legal |

## BLE protocol

Service UUID: `0000FEA5-...`

| Char | UUID suffix | Dir | Payload |
|---|---|---|---|
| cmd   | FEA6 | write  | JSON command |
| event | FEA7 | notify | JSON event stream |
| log   | FEA8 | notify | raw PCAP chunks |
| target| FEA9 | r/w    | HMAC-signed target blob |

## Legal

Authorized security research only. See `../README.md` for the full
disclaimer. © jayis1.