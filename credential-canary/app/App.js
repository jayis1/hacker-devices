// Credential Canary React Native companion
// Author: jayis1
// SPDX-License-Identifier: GPL-2.0-only
import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from "react";
import {
  SafeAreaView,
  View,
  Text,
  TouchableOpacity,
  FlatList,
  StyleSheet,
  TextInput,
  Alert as RNAlert,
  Share,
  Platform,
} from "react-native";
import { BleManager } from "react-native-ble-plx";
import { decode as b64decode, encode as b64encode } from "base-64";
import {
  Command,
  Mode,
  packet,
  parsePacket,
  decodeEvent,
  statusFromPayload,
} from "./src/protocol";

const SERVICE = "ca7a0001-4a41-5949-5331-435245444954";
const RX = "ca7a0002-4a41-5949-5331-435245444954";
const TX = "ca7a0003-4a41-5949-5331-435245444954";
const modes = ["SAFE BYPASS", "MONITOR", "BRIDGE", "LAB TEST"];
const bytesToB64 = (bytes) => b64encode(String.fromCharCode(...bytes));
const b64ToBytes = (value) =>
  Uint8Array.from(b64decode(value), (c) => c.charCodeAt(0));

function Pill({ label, active, onPress, danger }) {
  return (
    <TouchableOpacity
      onPress={onPress}
      style={[s.pill, active && s.pillOn, danger && s.danger]}
    >
      <Text style={s.pillText}>{label}</Text>
    </TouchableOpacity>
  );
}
function Metric({ name, value, tone }) {
  return (
    <View style={s.metric}>
      <Text style={s.metricName}>{name}</Text>
      <Text style={[s.metricValue, tone && { color: tone }]}>{value}</Text>
    </View>
  );
}

export default function App() {
  const manager = useRef(new BleManager()).current;
  const [tab, setTab] = useState("Overview");
  const [devices, setDevices] = useState([]);
  const [device, setDevice] = useState(null);
  const [events, setEvents] = useState([]);
  const [status, setStatus] = useState({
    mode: 0,
    streaming: false,
    tamper: false,
    alerts: 0,
    drops: 0,
    queued: 0,
    uptimeMs: 0,
  });
  const [scanning, setScanning] = useState(false);
  const [filter, setFilter] = useState("All");
  const [marker, setMarker] = useState("");
  const monitor = useRef(null);
  useEffect(
    () => () => {
      monitor.current?.remove();
      manager.destroy();
    },
    [manager],
  );
  const send = useCallback(
    async (command, payload = []) => {
      if (!device) throw new Error("Connect to a Canary first");
      const p = packet(command, payload);
      await device.writeCharacteristicWithResponseForService(
        SERVICE,
        RX,
        bytesToB64(p),
      );
    },
    [device],
  );
  const handleValue = useCallback((error, characteristic) => {
    if (error) {
      RNAlert.alert("Link error", error.message);
      return;
    }
    try {
      const frame = parsePacket(b64ToBytes(characteristic.value));
      if (frame.type === 0xf0)
        setEvents((old) => [decodeEvent(frame.payload), ...old].slice(0, 2000));
      else if (frame.type === (Command.STATUS | 0x80))
        setStatus(statusFromPayload(frame.payload));
    } catch (e) {
      console.warn("Canary frame rejected", e.message);
    }
  }, []);
  const scan = useCallback(async () => {
    setDevices([]);
    setScanning(true);
    const state = await manager.state();
    if (state !== "PoweredOn") {
      RNAlert.alert("Bluetooth unavailable", `Adapter state: ${state}`);
      setScanning(false);
      return;
    }
    manager.startDeviceScan([SERVICE], null, (error, d) => {
      if (error) {
        setScanning(false);
        RNAlert.alert("Scan failed", error.message);
        return;
      }
      if (d)
        setDevices((old) =>
          old.some((x) => x.id === d.id) ? old : [...old, d],
        );
    });
    setTimeout(() => {
      manager.stopDeviceScan();
      setScanning(false);
    }, 7000);
  }, [manager]);
  const connect = useCallback(
    async (d) => {
      try {
        manager.stopDeviceScan();
        const linked = await d.connect();
        await linked.discoverAllServicesAndCharacteristics();
        monitor.current = linked.monitorCharacteristicForService(
          SERVICE,
          TX,
          handleValue,
        );
        setDevice(linked);
        setTab("Overview");
        await linked.writeCharacteristicWithResponseForService(
          SERVICE,
          RX,
          bytesToB64(packet(Command.STATUS)),
        );
      } catch (e) {
        RNAlert.alert("Connection failed", e.message);
      }
    },
    [handleValue, manager],
  );
  const setMode = async (mode) => {
    if (mode >= Mode.BRIDGE) {
      RNAlert.alert(
        "Confirm authorized lab use",
        `Changing to ${modes[mode]} can alter the inline link. The hardware authorization button must also be held.`,
        [
          { text: "Cancel" },
          {
            text: "Continue",
            onPress: () =>
              send(Command.MODE, [mode])
                .then(() => setStatus((x) => ({ ...x, mode })))
                .catch((e) => RNAlert.alert("Mode failed", e.message)),
          },
        ],
      );
    } else {
      await send(Command.MODE, [mode]);
      setStatus((x) => ({ ...x, mode }));
    }
  };
  const toggleCapture = async () => {
    await send(status.streaming ? Command.STOP : Command.START);
    setStatus((x) => ({ ...x, streaming: !x.streaming }));
  };
  const addMarker = async () => {
    const bytes = Array.from(marker.slice(0, 32), (c) => c.charCodeAt(0) & 255);
    await send(Command.MARK, bytes);
    setMarker("");
  };
  const exportSession = async () => {
    const document = JSON.stringify(
      {
        device: device?.id,
        author: "jayis1",
        exportedAt: new Date().toISOString(),
        status,
        events: [...events].reverse(),
      },
      null,
      2,
    );
    await Share.share({
      title: "Credential Canary capture",
      message: document,
    });
  };
  const filtered = useMemo(
    () =>
      events.filter(
        (e) =>
          filter === "All" ||
          (filter === "Alerts" ? e.type === 8 : e.typeName.includes(filter)),
      ),
    [events, filter],
  );
  const alertEvents = events.filter((e) => e.type === 8);
  const Overview = () => (
    <View style={s.body}>
      <Text style={s.heading}>LINK HEALTH</Text>
      <View style={s.grid}>
        <Metric name="Mode" value={modes[status.mode]} />
        <Metric
          name="Capture"
          value={status.streaming ? "LIVE" : "STOPPED"}
          tone={status.streaming ? "#5ef38c" : "#90a0ad"}
        />
        <Metric
          name="Alerts"
          value={status.alerts}
          tone={status.alerts ? "#ff6577" : "#5ef38c"}
        />
        <Metric
          name="Dropped"
          value={status.drops}
          tone={status.drops ? "#ffb347" : "#5ef38c"}
        />
      </View>
      <Text style={s.heading}>OPERATING MODE</Text>
      <View style={s.wrap}>
        {modes.map((m, i) => (
          <Pill
            key={m}
            label={m}
            active={status.mode === i}
            danger={i > 1}
            onPress={() =>
              setMode(i).catch((e) =>
                RNAlert.alert("Command failed", e.message),
              )
            }
          />
        ))}
      </View>
      <Text style={s.heading}>SESSION</Text>
      <TouchableOpacity
        style={s.primary}
        onPress={() =>
          toggleCapture().catch((e) =>
            RNAlert.alert("Capture failed", e.message),
          )
        }
      >
        <Text style={s.primaryText}>
          {status.streaming ? "STOP CAPTURE" : "START CAPTURE"}
        </Text>
      </TouchableOpacity>
      <View style={s.row}>
        <TextInput
          value={marker}
          onChangeText={setMarker}
          placeholder="Evidence marker"
          placeholderTextColor="#65737d"
          style={s.input}
        />
        <Pill
          label="MARK"
          onPress={() =>
            addMarker().catch((e) => RNAlert.alert("Marker failed", e.message))
          }
        />
      </View>
      <Text style={s.note}>
        Active bridge and lab modes require explicit authorization and the
        device's physical enable control. Safe bypass is the power-loss default.
      </Text>
    </View>
  );
  const Capture = () => (
    <View style={s.body}>
      <View style={s.wrap}>
        {["All", "Wiegand", "OSDP", "Alerts"].map((x) => (
          <Pill
            key={x}
            label={x}
            active={filter === x}
            onPress={() => setFilter(x)}
          />
        ))}
      </View>
      <FlatList
        data={filtered}
        keyExtractor={(e) => String(e.sequence)}
        ListEmptyComponent={
          <Text style={s.empty}>No matching capture events.</Text>
        }
        renderItem={({ item: e }) => (
          <View style={[s.event, e.type === 8 && s.eventAlert]}>
            <View style={s.eventTop}>
              <Text style={s.eventType}>{e.alert || e.typeName}</Text>
              <Text style={s.time}>
                {(e.timestampUs / 1000000).toFixed(3)}s
              </Text>
            </View>
            <Text style={s.eventDetail}>
              {e.interface}
              {e.bits
                ? ` · ${e.bits} bit · credential …${e.credential.slice(-8)}`
                : ""}
              {e.command
                ? ` · addr ${e.address} · ${e.command} · ${e.secure ? "secure" : "clear"}`
                : ""}
              {e.marker ? ` · ${e.marker}` : ""}
            </Text>
          </View>
        )}
      />
    </View>
  );
  const Findings = () => (
    <View style={s.body}>
      <Text style={s.heading}>POLICY FINDINGS</Text>
      <Metric
        name="Session findings"
        value={alertEvents.length}
        tone={alertEvents.length ? "#ff6577" : "#5ef38c"}
      />
      {alertEvents.length === 0 ? (
        <Text style={s.empty}>No policy violations observed.</Text>
      ) : (
        alertEvents.map((e) => (
          <View key={e.sequence} style={[s.event, s.eventAlert]}>
            <Text style={s.eventType}>{e.alert}</Text>
            <Text style={s.eventDetail}>
              {e.interface} at {(e.timestampUs / 1000000).toFixed(3)} seconds
            </Text>
          </View>
        ))
      )}
      <TouchableOpacity
        style={s.secondary}
        onPress={() =>
          send(Command.RESET)
            .then(() => setEvents((x) => x.filter((e) => e.type !== 8)))
            .catch((e) => RNAlert.alert("Reset failed", e.message))
        }
      >
        <Text style={s.primaryText}>RESET POLICY BASELINE</Text>
      </TouchableOpacity>
      <TouchableOpacity style={s.primary} onPress={exportSession}>
        <Text style={s.primaryText}>EXPORT SIGNED WORKING COPY</Text>
      </TouchableOpacity>
    </View>
  );
  const Connect = () => (
    <View style={s.body}>
      <TouchableOpacity style={s.primary} onPress={scan}>
        <Text style={s.primaryText}>
          {scanning ? "SCANNING…" : "SCAN FOR CANARY"}
        </Text>
      </TouchableOpacity>
      {devices.map((d) => (
        <TouchableOpacity
          key={d.id}
          style={s.device}
          onPress={() => connect(d)}
        >
          <Text style={s.eventType}>{d.name || "Credential Canary"}</Text>
          <Text style={s.eventDetail}>
            {d.id} · RSSI {d.rssi ?? "?"}
          </Text>
        </TouchableOpacity>
      ))}
      <Text style={s.note}>
        Android requires Bluetooth scan/connect permission. iOS prompts on first
        scan. BLE traffic is intended for authenticated bonded links; use USB
        for evidentiary collection in RF-restricted facilities.
      </Text>
    </View>
  );
  const content = !device ? (
    <Connect />
  ) : tab === "Overview" ? (
    <Overview />
  ) : tab === "Capture" ? (
    <Capture />
  ) : (
    <Findings />
  );
  return (
    <SafeAreaView style={s.app}>
      <View style={s.header}>
        <View>
          <Text style={s.title}>CREDENTIAL CANARY</Text>
          <Text style={s.subtitle}>
            {device
              ? `CONNECTED · ${device.name || device.id.slice(0, 8)}`
              : "OFFLINE · jayis1"}
          </Text>
        </View>
        {device && (
          <TouchableOpacity
            onPress={() =>
              device.cancelConnection().finally(() => setDevice(null))
            }
          >
            <Text style={s.disconnect}>DISCONNECT</Text>
          </TouchableOpacity>
        )}
      </View>
      {content}
      {device && (
        <View style={s.tabs}>
          {["Overview", "Capture", "Findings"].map((x) => (
            <TouchableOpacity
              key={x}
              onPress={() => setTab(x)}
              style={[s.tab, tab === x && s.tabOn]}
            >
              <Text style={s.tabText}>{x.toUpperCase()}</Text>
            </TouchableOpacity>
          ))}
        </View>
      )}
    </SafeAreaView>
  );
}
const s = StyleSheet.create({
  app: { flex: 1, backgroundColor: "#071014" },
  header: {
    padding: 18,
    borderBottomWidth: 1,
    borderBottomColor: "#1c343d",
    flexDirection: "row",
    justifyContent: "space-between",
  },
  title: {
    color: "#d9ff47",
    fontWeight: "900",
    fontSize: 19,
    letterSpacing: 1.8,
  },
  subtitle: { color: "#6e8c96", fontSize: 11, marginTop: 4 },
  disconnect: { color: "#ff6577", fontSize: 11 },
  body: { flex: 1, padding: 16 },
  heading: {
    color: "#6e8c96",
    fontSize: 11,
    fontWeight: "800",
    letterSpacing: 1.5,
    marginTop: 12,
    marginBottom: 8,
  },
  grid: { flexDirection: "row", flexWrap: "wrap", gap: 8 },
  metric: {
    backgroundColor: "#0e1d22",
    borderWidth: 1,
    borderColor: "#1c343d",
    padding: 12,
    width: "47%",
    borderRadius: 6,
  },
  metricName: { color: "#78909a", fontSize: 11 },
  metricValue: {
    color: "#e6f1f3",
    fontSize: 17,
    fontWeight: "800",
    marginTop: 5,
  },
  wrap: { flexDirection: "row", flexWrap: "wrap", gap: 7 },
  pill: {
    borderWidth: 1,
    borderColor: "#36515a",
    paddingVertical: 8,
    paddingHorizontal: 11,
    borderRadius: 4,
  },
  pillOn: { backgroundColor: "#355000", borderColor: "#d9ff47" },
  danger: { borderColor: "#75404a" },
  pillText: { color: "#e6f1f3", fontSize: 11, fontWeight: "700" },
  primary: {
    backgroundColor: "#526f00",
    borderWidth: 1,
    borderColor: "#d9ff47",
    padding: 13,
    alignItems: "center",
    borderRadius: 4,
    marginVertical: 8,
  },
  secondary: {
    backgroundColor: "#25333a",
    padding: 13,
    alignItems: "center",
    borderRadius: 4,
    marginVertical: 8,
  },
  primaryText: {
    color: "#fff",
    fontWeight: "800",
    fontSize: 12,
    letterSpacing: 1,
  },
  row: { flexDirection: "row", alignItems: "center", gap: 8 },
  input: {
    flex: 1,
    color: "#fff",
    borderWidth: 1,
    borderColor: "#36515a",
    padding: 10,
    borderRadius: 4,
  },
  note: { color: "#738891", fontSize: 12, lineHeight: 18, marginTop: 14 },
  event: { padding: 12, borderBottomWidth: 1, borderBottomColor: "#193039" },
  eventAlert: {
    borderLeftWidth: 3,
    borderLeftColor: "#ff6577",
    backgroundColor: "#1c1216",
  },
  eventTop: { flexDirection: "row", justifyContent: "space-between" },
  eventType: { color: "#e6f1f3", fontWeight: "700" },
  eventDetail: { color: "#78909a", fontSize: 12, marginTop: 5 },
  time: { color: "#6e8c96", fontVariant: ["tabular-nums"] },
  empty: { color: "#58717a", textAlign: "center", marginTop: 40 },
  device: {
    padding: 14,
    borderWidth: 1,
    borderColor: "#1c343d",
    marginTop: 8,
    borderRadius: 4,
  },
  tabs: { flexDirection: "row", borderTopWidth: 1, borderTopColor: "#1c343d" },
  tab: { flex: 1, alignItems: "center", padding: 15 },
  tabOn: { borderTopWidth: 2, borderTopColor: "#d9ff47" },
  tabText: { color: "#9bb0b7", fontSize: 10, fontWeight: "800" },
});
