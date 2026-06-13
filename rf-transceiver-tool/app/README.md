# RF Transceiver Tool — Companion App

React Native companion app for the RF Transceiver Tool. Communicates with the device over USB CDC (virtual serial port) for real-time control, monitoring, and packet capture.

## Features

- **Sub-GHz (CC1101)** — Configure frequency, modulation, data rate, TX power, sync word
- **2.4 GHz (nRF24L01+)** — Configure channel, data rate, TX power, pipe addresses
- **Real-time packet log** — Captured packets displayed in hex with RSSI/LQI
- **Quick presets** — Common frequencies (433.92 MHz, 868 MHz, 915 MHz) and channels
- **Status dashboard** — Connection state, operating mode, RSSI indicator

## Setup

### Prerequisites

- Node.js 16+
- React Native CLI
- Android SDK (for Android) or Xcode (for iOS)
- USB OTG cable (for Android device connection)

### Install

```bash
cd app/
npm install
# or
yarn install
```

### Run on Android

```bash
npx react-native run-android
```

### Run on iOS

```bash
npx react-native run-ios
```

## Project Structure

```
app/
├── App.js                    — Main app with navigation
├── package.json              — Dependencies and scripts
├── navigation/
│   └── StackNavigator.js    — Navigation stack
├── screens/
│   ├── DeviceScreen.js       — Main device control screen
│   └── SettingsScreen.js     — RF configuration screen
├── components/
│   └── StatusCard.js         — Reusable status display card
└── utils/
    └── protocol.js           — Wire protocol definitions
```

## Wire Protocol

The app communicates with the firmware using a binary protocol over USB CDC:

```
[0xAA] [CMD_ID] [LEN] [PAYLOAD...] [CRC8]
```

See `firmware/README.md` and `utils/protocol.js` for the full command specification.

## Dependencies

| Package | Purpose |
|---|---|
| `react-navigation` | Screen navigation |
| `react-native-usb-serial` | USB CDC serial communication |
| `react-native-ble-plx` | BLE communication (future) |
| `react-native-vector-icons` | UI icons |

## License

GPL-2.0