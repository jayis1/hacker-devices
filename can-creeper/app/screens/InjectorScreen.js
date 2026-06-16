/**
 * InjectorScreen.js
 *
 * CAN frame injection interface for crafting and transmitting
 * arbitrary CAN/CAN FD frames. Supports manual frame construction,
 * ISO-TP segmented message sending, and UDS diagnostic requests.
 */

import React, { useState, useCallback } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  ScrollView,
  Switch,
  StyleSheet,
  Alert,
  KeyboardAvoidingView,
  Platform,
} from 'react-native';

/* UDS service presets */
const UDS_SERVICES = [
  { id: 0x10, name: 'Diagnostic Session Control', subfunctions: [
    { id: 0x01, name: 'Default Session' },
    { id: 0x02, name: 'Programming Session' },
    { id: 0x03, name: 'Extended Diagnostic Session' },
  ]},
  { id: 0x11, name: 'ECU Reset', subfunctions: [
    { id: 0x01, name: 'Hard Reset' },
    { id: 0x02, name: 'Key Off/On Reset' },
    { id: 0x03, name: 'Soft Reset' },
  ]},
  { id: 0x22, name: 'Read Data By Identifier', subfunctions: [] },
  { id: 0x27, name: 'Security Access', subfunctions: [
    { id: 0x01, name: 'Request Seed' },
    { id: 0x02, name: 'Send Key' },
  ]},
  { id: 0x2E, name: 'Write Data By Identifier', subfunctions: [] },
  { id: 0x34, name: 'Request Download', subfunctions: [] },
  { id: 0x35, name: 'Request Upload', subfunctions: [] },
  { id: 0x36, name: 'Transfer Data', subfunctions: [] },
  { id: 0x37, name: 'Request Transfer Exit', subfunctions: [] },
  { id: 0x3E, name: 'Tester Present', subfunctions: [
    { id: 0x00, name: 'Zero Subfunction' },
  ]},
];

/**
 * Injector Screen Component
 */
const InjectorScreen = ({ protocol, connectionState }) => {
  const [channel, setChannel] = useState(0);
  const [canId, setCanId] = useState('7E0');
  const [isExtendedId, setIsExtendedId] = useState(false);
  const [isFDFrame, setIsFDFrame] = useState(false);
  const [brsEnabled, setBrsEnabled] = useState(false);
  const [dataBytes, setDataBytes] = useState('02 27 01 00 00 00 00 00');
  const [injectionRate, setInjectionRate] = useState('10');
  const [repeatCount, setRepeatCount] = useState('1');
  const [isInjecting, setIsInjecting] = useState(false);
  const [txHistory, setTxHistory] = useState([]);
  const [selectedUdsService, setSelectedUdsService] = useState(null);
  const [udsSubfunction, setUdsSubfunction] = useState('');
  const [udsDid, setUdsDid] = useState('');

  const isConnected = connectionState.bleConnected || connectionState.usbConnected;

  /* Parse data bytes string to array */
  const parseDataBytes = useCallback((str) => {
    const hexBytes = str.trim().split(/[\s,]+/);
    return hexBytes.map(h => parseInt(h, 16) || 0);
  }, []);

  /* Build and send a single CAN frame */
  const sendFrame = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to CAN Creeper first.');
      return;
    }

    const id = parseInt(canId, 16);
    if (isNaN(id) || id < 0 || id > 0x1FFFFFFF) {
      Alert.alert('Invalid ID', 'CAN ID must be a valid hex value (0-0x1FFFFFFF).');
      return;
    }

    const data = parseDataBytes(dataBytes);
    if (data.length === 0) {
      Alert.alert('No Data', 'Enter at least one data byte.');
      return;
    }

    const dlc = data.length <= 8 ? data.length :
                data.length <= 12 ? 9 :
                data.length <= 16 ? 10 :
                data.length <= 20 ? 11 :
                data.length <= 24 ? 12 :
                data.length <= 32 ? 13 :
                data.length <= 48 ? 14 :
                data.length <= 64 ? 15 : 15;

    const frame = {
      channel,
      id,
      ide: isExtendedId ? 1 : 0,
      fd: isFDFrame ? 1 : 0,
      brs: brsEnabled ? 1 : 0,
      esi: 0,
      rtr: 0,
      dlc,
      data: data.concat(new Array(64 - data.length).fill(0)),
      timestamp: 0,
    };

    try {
      await protocol.sendFrame(frame);
      setTxHistory(prev => [{
        timestamp: Date.now(),
        id,
        dlc,
        data: data.slice(0, 8),
        channel,
      }, ...prev].slice(0, 50));
    } catch (error) {
      Alert.alert('TX Error', error.message);
    }
  }, [isConnected, canId, dataBytes, channel, isExtendedId, isFDFrame, brsEnabled, parseDataBytes]);

  /* Start repeated injection */
  const startInjection = useCallback(async () => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to CAN Creeper first.');
      return;
    }

    const rate = parseInt(injectionRate, 10);
    const repeat = parseInt(repeatCount, 10);
    if (isNaN(rate) || rate < 1 || rate > 10000) {
      Alert.alert('Invalid Rate', 'Rate must be 1-10000 frames per second.');
      return;
    }

    setIsInjecting(true);

    const id = parseInt(canId, 16);
    const data = parseDataBytes(dataBytes);
    const dlc = data.length <= 8 ? data.length : 9;

    try {
      await protocol.startInjection({
        channel,
        frames: [{
          id,
          ide: isExtendedId ? 1 : 0,
          dlc,
          data,
          fd: isFDFrame ? 1 : 0,
          brs: brsEnabled ? 1 : 0,
        }],
        rate,
        repeat: repeat === 0 ? 0 : repeat,  /* 0 = infinite */
      });
    } catch (error) {
      Alert.alert('Injection Error', error.message);
      setIsInjecting(false);
    }
  }, [isConnected, canId, dataBytes, channel, injectionRate, repeatCount, isExtendedId, isFDFrame, brsEnabled]);

  /* Stop injection */
  const stopInjection = useCallback(async () => {
    try {
      await protocol.stopInjection();
    } catch (error) {
      Alert.alert('Stop Error', error.message);
    }
    setIsInjecting(false);
  }, []);

  /* Build UDS request */
  const buildUdsRequest = useCallback(() => {
    if (!selectedUdsService) return;

    let data = [selectedUdsService.id];
    if (udsSubfunction) {
      data.push(parseInt(udsSubfunction, 16) || 0);
    }
    if (udsDid && (selectedUdsService.id === 0x22 || selectedUdsService.id === 0x2E)) {
      const did = parseInt(udsDid, 16);
      data.push((did >> 8) & 0xFF);
      data.push(did & 0xFF);
    }
    /* Pad to 8 bytes */
    while (data.length < 8) data.push(0x00);
    setDataBytes(data.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' '));
  }, [selectedUdsService, udsSubfunction, udsDid]);

  /* Build ISO-TP flow control response */
  const buildIsoTpFrame = useCallback((type, data) => {
    if (type === 'SF') {
      /* Single Frame: PCI byte = length */
      const pci = data.length;
      const full = [pci, ...data];
      while (full.length < 8) full.push(0xAA);
      setDataBytes(full.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' '));
    } else if (type === 'FF') {
      /* First Frame: PCI high = 0x10 | (len >> 8), PCI low = len & 0xFF */
      const len = data.length;
      const full = [0x10 | ((len >> 8) & 0x0F), len & 0xFF, ...data.slice(0, 6)];
      while (full.length < 8) full.push(0xAA);
      setDataBytes(full.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' '));
    } else if (type === 'CF') {
      /* Consecutive Frame: PCI = 0x20 | seq */
      const seq = 1;
      const full = [0x20 | (seq & 0x0F), ...data.slice(0, 7)];
      while (full.length < 8) full.push(0xAA);
      setDataBytes(full.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' '));
    }
  }, []);

  return (
    <KeyboardAvoidingView
      style={styles.container}
      behavior={Platform.OS === 'ios' ? 'padding' : 'height'}
    >
      <ScrollView style={styles.scrollView} contentContainerStyle={styles.scrollContent}>
        {/* Connection warning */}
        {!isConnected && (
          <View style={styles.warningBanner}>
            <Text style={styles.warningText}>
              ⚠ Not connected to CAN Creeper. Connect via BLE or USB to inject frames.
            </Text>
          </View>
        )}

        {/* Channel selector */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Channel</Text>
          <View style={styles.channelRow}>
            <TouchableOpacity
              style={[styles.channelBtn, channel === 0 && styles.channelBtnActive]}
              onPress={() => setChannel(0)}
            >
              <Text style={[styles.channelBtnText, channel === 0 && styles.channelBtnTextActive]}>
                CAN 0
              </Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={[styles.channelBtn, channel === 1 && styles.channelBtnActive]}
              onPress={() => setChannel(1)}
            >
              <Text style={[styles.channelBtnText, channel === 1 && styles.channelBtnTextActive]}>
                CAN 1
              </Text>
            </TouchableOpacity>
          </View>
        </View>

        {/* Frame configuration */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Frame Configuration</Text>

          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>CAN ID (hex):</Text>
            <TextInput
              style={styles.input}
              value={canId}
              onChangeText={setCanId}
              placeholder="7E0"
              placeholderTextColor="#484F58"
              autoCapitalize="none"
              autoCorrect={false}
            />
          </View>

          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>Extended ID (29-bit)</Text>
            <Switch
              value={isExtendedId}
              onValueChange={setIsExtendedId}
              trackColor={{ false: '#30363D', true: '#FF6B0050' }}
              thumbColor={isExtendedId ? '#FF6B00' : '#8B949E'}
            />
          </View>

          <View style={styles.switchRow}>
            <Text style={styles.switchLabel}>CAN FD Frame</Text>
            <Switch
              value={isFDFrame}
              onValueChange={setIsFDFrame}
              trackColor={{ false: '#30363D', true: '#FF6B0050' }}
              thumbColor={isFDFrame ? '#FF6B00' : '#8B949E'}
            />
          </View>

          {isFDFrame && (
            <View style={styles.switchRow}>
              <Text style={styles.switchLabel}>Bit Rate Switch (BRS)</Text>
              <Switch
                value={brsEnabled}
                onValueChange={setBrsEnabled}
                trackColor={{ false: '#30363D', true: '#FF6B0050' }}
                thumbColor={brsEnabled ? '#FF6B00' : '#8B949E'}
              />
            </View>
          )}

          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Data Bytes (hex):</Text>
            <TextInput
              style={[styles.input, styles.dataInput]}
              value={dataBytes}
              onChangeText={setDataBytes}
              placeholder="02 27 01 00 00 00 00 00"
              placeholderTextColor="#484F58"
              autoCapitalize="none"
              autoCorrect={false}
              multiline
              numberOfLines={3}
            />
          </View>
        </View>

        {/* Quick-build buttons */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Quick Build</Text>
          <View style={styles.quickBuildRow}>
            <TouchableOpacity
              style={styles.quickBuildBtn}
              onPress={() => buildIsoTpFrame('SF', [0x22, 0xF1, 0x90])}
            >
              <Text style={styles.quickBuildText}>ISO-TP SF</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.quickBuildBtn}
              onPress={() => buildIsoTpFrame('FF', new Array(20).fill(0xAA))}
            >
              <Text style={styles.quickBuildText}>ISO-TP FF</Text>
            </TouchableOpacity>
            <TouchableOpacity
              style={styles.quickBuildBtn}
              onPress={() => buildIsoTpFrame('CF', [0xAA, 0xBB, 0xCC])}
            >
              <Text style={styles.quickBuildText}>ISO-TP CF</Text>
            </TouchableOpacity>
          </View>
        </View>

        {/* UDS presets */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>UDS Diagnostic Requests</Text>
          <ScrollView horizontal showsHorizontalScrollIndicator={false}>
            <View style={styles.udsRow}>
              {UDS_SERVICES.map(svc => (
                <TouchableOpacity
                  key={svc.id}
                  style={[
                    styles.udsBtn,
                    selectedUdsService?.id === svc.id && styles.udsBtnActive,
                  ]}
                  onPress={() => {
                    setSelectedUdsService(svc);
                    setUdsSubfunction('');
                    setUdsDid('');
                  }}
                >
                  <Text style={[
                    styles.udsBtnText,
                    selectedUdsService?.id === svc.id && styles.udsBtnTextActive,
                  ]}>
                    0x{svc.id.toString(16).toUpperCase().padStart(2, '0')}
                  </Text>
                  <Text style={styles.udsBtnSubtext}>{svc.name}</Text>
                </TouchableOpacity>
              ))}
            </View>
          </ScrollView>

          {selectedUdsService && selectedUdsService.subfunctions.length > 0 && (
            <View style={styles.subfunctionRow}>
              <Text style={styles.subfunctionLabel}>Subfunction:</Text>
              <TextInput
                style={styles.subfunctionInput}
                value={udsSubfunction}
                onChangeText={setUdsSubfunction}
                placeholder="01"
                placeholderTextColor="#484F58"
                autoCapitalize="none"
              />
              <ScrollView horizontal style={styles.subfunctionList}>
                {selectedUdsService.subfunctions.map(sf => (
                  <TouchableOpacity
                    key={sf.id}
                    style={styles.subfunctionBtn}
                    onPress={() => setUdsSubfunction(sf.id.toString(16).toUpperCase().padStart(2, '0'))}
                  >
                    <Text style={styles.subfunctionBtnText}>{sf.name}</Text>
                  </TouchableOpacity>
                ))}
              </ScrollView>
            </View>
          )}

          {(selectedUdsService?.id === 0x22 || selectedUdsService?.id === 0x2E) && (
            <View style={styles.inputRow}>
              <Text style={styles.inputLabel}>DID (hex):</Text>
              <TextInput
                style={styles.input}
                value={udsDid}
                onChangeText={setUdsDid}
                placeholder="F190"
                placeholderTextColor="#484F58"
                autoCapitalize="none"
              />
            </View>
          )}

          {selectedUdsService && (
            <TouchableOpacity style={styles.buildBtn} onPress={buildUdsRequest}>
              <Text style={styles.buildBtnText}>Build UDS Request</Text>
            </TouchableOpacity>
          )}
        </View>

        {/* Injection controls */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Injection Control</Text>

          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Rate (frames/sec):</Text>
            <TextInput
              style={styles.input}
              value={injectionRate}
              onChangeText={setInjectionRate}
              placeholder="10"
              placeholderTextColor="#484F58"
              keyboardType="numeric"
            />
          </View>

          <View style={styles.inputRow}>
            <Text style={styles.inputLabel}>Repeat (0=infinite):</Text>
            <TextInput
              style={styles.input}
              value={repeatCount}
              onChangeText={setRepeatCount}
              placeholder="1"
              placeholderTextColor="#484F58"
              keyboardType="numeric"
            />
          </View>

          <View style={styles.injectionBtnRow}>
            <TouchableOpacity
              style={[styles.sendBtn, !isConnected && styles.btnDisabled]}
              onPress={sendFrame}
              disabled={!isConnected}
            >
              <Text style={styles.sendBtnText}>Send Single Frame</Text>
            </TouchableOpacity>

            {!isInjecting ? (
              <TouchableOpacity
                style={[styles.injectBtn, !isConnected && styles.btnDisabled]}
                onPress={startInjection}
                disabled={!isConnected}
              >
                <Text style={styles.injectBtnText}>▶ Start Injection</Text>
              </TouchableOpacity>
            ) : (
              <TouchableOpacity style={styles.stopBtn} onPress={stopInjection}>
                <Text style={styles.stopBtnText}>⏹ Stop Injection</Text>
              </TouchableOpacity>
            )}
          </View>

          {isInjecting && (
            <View style={styles.injectingBanner}>
              <Text style={styles.injectingText}>
                🔄 Injecting at {injectionRate} fps on CH{channel}...
              </Text>
            </View>
          )}
        </View>

        {/* TX History */}
        <View style={styles.section}>
          <Text style={styles.sectionTitle}>TX History</Text>
          {txHistory.length === 0 ? (
            <Text style={styles.emptyHistory}>No frames transmitted yet.</Text>
          ) : (
            txHistory.map((entry, idx) => (
              <View key={idx} style={styles.historyItem}>
                <Text style={styles.historyId}>
                  0x{entry.id.toString(16).toUpperCase().padStart(3, '0')}
                </Text>
                <Text style={styles.historyMeta}>
                  CH{entry.channel} DLC:{entry.dlc}
                </Text>
                <Text style={styles.historyData}>
                  {entry.data.map(b => b.toString(16).toUpperCase().padStart(2, '0')).join(' ')}
                </Text>
              </View>
            ))
          )}
        </View>
      </ScrollView>
    </KeyboardAvoidingView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D1117',
  },
  scrollView: {
    flex: 1,
  },
  scrollContent: {
    paddingBottom: 40,
  },
  warningBanner: {
    backgroundColor: '#FF6B0020',
    borderWidth: 1,
    borderColor: '#FF6B00',
    borderRadius: 8,
    padding: 12,
    margin: 12,
  },
  warningText: {
    color: '#FF6B00',
    fontSize: 13,
    textAlign: 'center',
  },
  section: {
    backgroundColor: '#161B22',
    marginHorizontal: 12,
    marginTop: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#30363D',
    padding: 12,
  },
  sectionTitle: {
    color: '#E6EDF3',
    fontSize: 15,
    fontWeight: '700',
    marginBottom: 10,
  },
  channelRow: {
    flexDirection: 'row',
  },
  channelBtn: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
    backgroundColor: '#21262D',
    marginHorizontal: 4,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  channelBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  channelBtnText: {
    color: '#8B949E',
    fontSize: 14,
    fontWeight: '600',
  },
  channelBtnTextActive: {
    color: '#FF6B00',
  },
  inputRow: {
    marginBottom: 10,
  },
  inputLabel: {
    color: '#8B949E',
    fontSize: 12,
    fontWeight: '600',
    marginBottom: 4,
  },
  input: {
    backgroundColor: '#0D1117',
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 8,
    color: '#E6EDF3',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  dataInput: {
    minHeight: 60,
    textAlignVertical: 'top',
  },
  switchRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 10,
  },
  switchLabel: {
    color: '#E6EDF3',
    fontSize: 13,
  },
  quickBuildRow: {
    flexDirection: 'row',
  },
  quickBuildBtn: {
    flex: 1,
    paddingVertical: 8,
    alignItems: 'center',
    backgroundColor: '#21262D',
    marginHorizontal: 3,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
  },
  quickBuildText: {
    color: '#58A6FF',
    fontSize: 12,
    fontWeight: '600',
  },
  udsRow: {
    flexDirection: 'row',
    paddingVertical: 4,
  },
  udsBtn: {
    paddingHorizontal: 10,
    paddingVertical: 8,
    backgroundColor: '#21262D',
    marginRight: 6,
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#30363D',
    alignItems: 'center',
    minWidth: 80,
  },
  udsBtnActive: {
    backgroundColor: '#FF6B0020',
    borderColor: '#FF6B00',
  },
  udsBtnText: {
    color: '#8B949E',
    fontSize: 13,
    fontWeight: '700',
    fontFamily: 'monospace',
  },
  udsBtnTextActive: {
    color: '#FF6B00',
  },
  udsBtnSubtext: {
    color: '#484F58',
    fontSize: 9,
    marginTop: 2,
  },
  subfunctionRow: {
    marginTop: 8,
  },
  subfunctionLabel: {
    color: '#8B949E',
    fontSize: 12,
    marginBottom: 4,
  },
  subfunctionInput: {
    backgroundColor: '#0D1117',
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    color: '#E6EDF3',
    fontSize: 13,
    fontFamily: 'monospace',
    marginBottom: 6,
  },
  subfunctionList: {
    flexDirection: 'row',
  },
  subfunctionBtn: {
    paddingHorizontal: 8,
    paddingVertical: 4,
    backgroundColor: '#21262D',
    marginRight: 4,
    borderRadius: 4,
  },
  subfunctionBtnText: {
    color: '#8B949E',
    fontSize: 10,
  },
  buildBtn: {
    marginTop: 8,
    paddingVertical: 8,
    alignItems: 'center',
    backgroundColor: '#FF6B0020',
    borderRadius: 6,
    borderWidth: 1,
    borderColor: '#FF6B00',
  },
  buildBtnText: {
    color: '#FF6B00',
    fontSize: 13,
    fontWeight: '600',
  },
  injectionBtnRow: {
    flexDirection: 'row',
    marginTop: 8,
  },
  sendBtn: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#238636',
    marginRight: 4,
    borderRadius: 6,
  },
  sendBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  injectBtn: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#FF6B00',
    marginLeft: 4,
    borderRadius: 6,
  },
  injectBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  stopBtn: {
    flex: 1,
    paddingVertical: 10,
    alignItems: 'center',
    backgroundColor: '#DA3633',
    marginLeft: 4,
    borderRadius: 6,
  },
  stopBtnText: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: '700',
  },
  btnDisabled: {
    opacity: 0.4,
  },
  injectingBanner: {
    marginTop: 8,
    backgroundColor: '#FF6B0020',
    borderRadius: 6,
    padding: 8,
    alignItems: 'center',
  },
  injectingText: {
    color: '#FF6B00',
    fontSize: 13,
    fontWeight: '600',
  },
  emptyHistory: {
    color: '#484F58',
    fontSize: 13,
    textAlign: 'center',
    paddingVertical: 20,
  },
  historyItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#21262D',
  },
  historyId: {
    color: '#FF6B00',
    fontSize: 12,
    fontWeight: '700',
    fontFamily: 'monospace',
    width: 55,
  },
  historyMeta: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
    width: 70,
  },
  historyData: {
    color: '#7EE787',
    fontSize: 10,
    fontFamily: 'monospace',
    flex: 1,
  },
});

export default InjectorScreen;
