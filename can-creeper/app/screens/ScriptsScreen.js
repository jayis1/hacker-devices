/**
 * ScriptsScreen.js
 *
 * Injection script management interface. Allows loading, editing,
 * and executing JSON-based CAN injection scripts for automated
 * fuzzing, replay attacks, and diagnostic probing sequences.
 */

import React, { useState, useCallback, useEffect } from 'react';
import {
  View,
  Text,
  TouchableOpacity,
  FlatList,
  TextInput,
  Alert,
  StyleSheet,
  ScrollView,
  ActivityIndicator,
} from 'react-native';

/* Built-in script templates */
const BUILTIN_SCRIPTS = [
  {
    name: 'UDS Security Access (Request Seed)',
    description: 'Sends Security Access request to ECU 0x7E0, waits for response',
    channel: 0,
    bitrate: 500000,
    delay_us: 100,
    repeat: 1,
    frames: [
      { id: 0x7E0, ide: 0, dlc: 8, data: [0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00], delay_after_us: 50000 },
    ],
  },
  {
    name: 'CAN ID Scanner (11-bit)',
    description: 'Scans all 2048 standard CAN IDs, sending one frame per ID',
    channel: 0,
    bitrate: 500000,
    delay_us: 2000,
    repeat: 1,
    frames: Array.from({ length: 10 }, (_, i) => ({
      id: 0x100 + i * 0x10,
      ide: 0,
      dlc: 8,
      data: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
      delay_after_us: 5000,
    })),
  },
  {
    name: 'Bus Load Test (10% at 500kbps)',
    description: 'Generates ~10% bus load with periodic frames',
    channel: 0,
    bitrate: 500000,
    delay_us: 2000,
    repeat: 0,
    frames: [
      { id: 0x123, ide: 0, dlc: 8, data: [0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11], delay_after_us: 2000 },
    ],
  },
  {
    name: 'CAN FD Discovery Probe',
    description: 'Attempts CAN FD communication to discover FD-capable ECUs',
    channel: 0,
    bitrate: 500000,
    fd_enable: true,
    delay_us: 500,
    repeat: 1,
    frames: [
      { id: 0x7DF, ide: 0, dlc: 12, fd: 1, brs: 1, data: new Array(12).fill(0xAA), delay_after_us: 10000 },
    ],
  },
];

/**
 * Scripts Screen Component
 */
const ScriptsScreen = ({ protocol, connectionState }) => {
  const [scripts, setScripts] = useState([]);
  const [activeScript, setActiveScript] = useState(null);
  const [scriptStatus, setScriptStatus] = useState(null);
  const [isLoading, setIsLoading] = useState(false);
  const [editorVisible, setEditorVisible] = useState(false);
  const [editorContent, setEditorContent] = useState('');
  const [editorName, setEditorName] = useState('');

  const isConnected = connectionState.bleConnected || connectionState.usbConnected;

  /* Load scripts from device on mount */
  useEffect(() => {
    if (isConnected) {
      loadScriptsFromDevice();
    }
  }, [isConnected]);

  /* Load built-in scripts as defaults */
  useEffect(() => {
    setScripts(BUILTIN_SCRIPTS.map((s, i) => ({
      ...s,
      _id: `builtin-${i}`,
      _type: 'builtin',
    })));
  }, []);

  const loadScriptsFromDevice = useCallback(async () => {
    setIsLoading(true);
    try {
      const deviceScripts = await protocol.listScripts();
      setScripts(prev => [
        ...prev.filter(s => s._type === 'builtin'),
        ...deviceScripts.map((s, i) => ({ ...s, _id: `device-${i}`, _type: 'device' })),
      ]);
    } catch (error) {
      console.log('Failed to load scripts from device:', error.message);
    }
    setIsLoading(false);
  }, []);

  /* Start a script */
  const handleStartScript = useCallback(async (script) => {
    if (!isConnected) {
      Alert.alert('Not Connected', 'Connect to CAN Creeper first.');
      return;
    }

    try {
      if (script._type === 'builtin') {
        /* Upload built-in script to device first */
        const scriptJson = JSON.stringify({
          name: script.name,
          channel: script.channel,
          bitrate: script.bitrate,
          fd_enable: script.fd_enable || false,
          delay_us: script.delay_us,
          repeat: script.repeat,
          frames: script.frames,
        });
        await protocol.uploadScript(scriptJson);
      }

      await protocol.startScript(script.name);
      setActiveScript(script);
      setScriptStatus({ state: 'running', step: 0, total: script.frames.length });

      /* Poll for status updates */
      const statusInterval = setInterval(async () => {
        try {
          const status = await protocol.getScriptStatus();
          setScriptStatus(status);
          if (!status.running) {
            clearInterval(statusInterval);
            setActiveScript(null);
          }
        } catch (e) {
          clearInterval(statusInterval);
        }
      }, 500);

      /* Store interval ID for cleanup */
      script._statusInterval = statusInterval;
    } catch (error) {
      Alert.alert('Script Error', error.message);
    }
  }, [isConnected]);

  /* Stop active script */
  const handleStopScript = useCallback(async () => {
    try {
      await protocol.stopScript();
      if (activeScript?._statusInterval) {
        clearInterval(activeScript._statusInterval);
      }
      setActiveScript(null);
      setScriptStatus(null);
    } catch (error) {
      Alert.alert('Stop Error', error.message);
    }
  }, [activeScript]);

  /* Open script editor */
  const handleEditScript = useCallback((script) => {
    const content = JSON.stringify({
      name: script.name,
      channel: script.channel,
      bitrate: script.bitrate,
      fd_enable: script.fd_enable || false,
      delay_us: script.delay_us,
      repeat: script.repeat,
      frames: script.frames,
    }, null, 2);
    setEditorName(script.name);
    setEditorContent(content);
    setEditorVisible(true);
  }, []);

  /* Save edited script */
  const handleSaveScript = useCallback(async () => {
    try {
      const parsed = JSON.parse(editorContent);
      if (!parsed.name || !parsed.frames) {
        Alert.alert('Invalid Script', 'Script must have a name and frames array.');
        return;
      }
      await protocol.uploadScript(editorContent);
      setEditorVisible(false);
      loadScriptsFromDevice();
    } catch (error) {
      Alert.alert('JSON Error', error.message);
    }
  }, [editorContent]);

  /* Create new script */
  const handleNewScript = useCallback(() => {
    const template = {
      name: 'New Script',
      channel: 0,
      bitrate: 500000,
      fd_enable: false,
      delay_us: 100,
      repeat: 1,
      frames: [
        { id: 0x7E0, ide: 0, dlc: 8, data: [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00], delay_after_us: 10000 },
      ],
    };
    setEditorName('New Script');
    setEditorContent(JSON.stringify(template, null, 2));
    setEditorVisible(true);
  }, []);

  /* Render script item */
  const renderScript = useCallback(({ item }) => {
    const isActive = activeScript?._id === item._id;
    const isBuiltin = item._type === 'builtin';

    return (
      <View style={[styles.scriptItem, isActive && styles.scriptItemActive]}>
        <View style={styles.scriptHeader}>
          <View style={styles.scriptInfo}>
            <Text style={styles.scriptName}>{item.name}</Text>
            <Text style={styles.scriptDesc}>{item.description || `${item.frames.length} frames, CH${item.channel}`}</Text>
            <View style={styles.scriptMeta}>
              <Text style={styles.metaText}>
                {item.frames.length} frames | {item.bitrate / 1000} kbps
                {item.fd_enable ? ' | FD' : ''} | Repeat: {item.repeat || '∞'}
              </Text>
            </View>
          </View>
          <View style={styles.scriptActions}>
            {isActive ? (
              <TouchableOpacity style={styles.stopBtn} onPress={handleStopScript}>
                <Text style={styles.stopBtnText}>⏹</Text>
              </TouchableOpacity>
            ) : (
              <TouchableOpacity
                style={[styles.startBtn, !isConnected && styles.btnDisabled]}
                onPress={() => handleStartScript(item)}
                disabled={!isConnected}
              >
                <Text style={styles.startBtnText}>▶</Text>
              </TouchableOpacity>
            )}
            <TouchableOpacity style={styles.editBtn} onPress={() => handleEditScript(item)}>
              <Text style={styles.editBtnText}>✏️</Text>
            </TouchableOpacity>
          </View>
        </View>

        {isActive && scriptStatus && (
          <View style={styles.statusBar}>
            <View style={styles.progressBar}>
              <View style={[styles.progressFill, {
                width: `${scriptStatus.total > 0
                  ? (scriptStatus.step / scriptStatus.total) * 100
                  : 0}%`,
              }]} />
            </View>
            <Text style={styles.statusText}>
              Step {scriptStatus.step}/{scriptStatus.total}
              {scriptStatus.paused ? ' (Paused)' : ''}
            </Text>
          </View>
        )}

        {isBuiltin && (
          <View style={styles.builtinBadge}>
            <Text style={styles.builtinText}>BUILT-IN</Text>
          </View>
        )}
      </View>
    );
  }, [activeScript, scriptStatus, isConnected, handleStartScript, handleStopScript, handleEditScript]);

  /* Script editor modal */
  if (editorVisible) {
    return (
      <View style={styles.container}>
        <View style={styles.editorHeader}>
          <TouchableOpacity onPress={() => setEditorVisible(false)}>
            <Text style={styles.cancelText}>Cancel</Text>
          </TouchableOpacity>
          <Text style={styles.editorTitle}>Edit Script</Text>
          <TouchableOpacity onPress={handleSaveScript}>
            <Text style={styles.saveText}>Save</Text>
          </TouchableOpacity>
        </View>
        <View style={styles.editorNameRow}>
          <Text style={styles.editorLabel}>Name:</Text>
          <TextInput
            style={styles.editorNameInput}
            value={editorName}
            onChangeText={setEditorName}
            placeholder="Script name"
            placeholderTextColor="#484F58"
          />
        </View>
        <TextInput
          style={styles.editorInput}
          value={editorContent}
          onChangeText={setEditorContent}
          multiline
          autoCapitalize="none"
          autoCorrect={false}
          textAlignVertical="top"
          fontFamily="monospace"
          spellCheck={false}
        />
        <View style={styles.editorHelp}>
          <Text style={styles.editorHelpText}>
            JSON format: name, channel, bitrate, fd_enable, delay_us, repeat, frames[]
          </Text>
          <Text style={styles.editorHelpText}>
            Each frame: id, ide, dlc, data[], delay_after_us, fd, brs
          </Text>
        </View>
      </View>
    );
  }

  return (
    <View style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <Text style={styles.headerTitle}>Injection Scripts</Text>
        <View style={styles.headerActions}>
          <TouchableOpacity style={styles.newBtn} onPress={handleNewScript}>
            <Text style={styles.newBtnText}>+ New</Text>
          </TouchableOpacity>
          {isConnected && (
            <TouchableOpacity style={styles.refreshBtn} onPress={loadScriptsFromDevice}>
              {isLoading ? (
                <ActivityIndicator color="#FF6B00" size="small" />
              ) : (
                <Text style={styles.refreshBtnText}>🔄</Text>
              )}
            </TouchableOpacity>
          )}
        </View>
      </View>

      {/* Active script status */}
      {activeScript && scriptStatus && (
        <View style={styles.activeBanner}>
          <Text style={styles.activeTitle}>▶ Running: {activeScript.name}</Text>
          <View style={styles.progressBarLarge}>
            <View style={[styles.progressFillLarge, {
              width: `${scriptStatus.total > 0
                ? (scriptStatus.step / scriptStatus.total) * 100
                : 0}%`,
            }]} />
          </View>
          <Text style={styles.activeDetail}>
            Step {scriptStatus.step}/{scriptStatus.total} | CH{activeScript.channel}
          </Text>
        </View>
      )}

      {/* Script list */}
      <FlatList
        data={scripts}
        renderItem={renderScript}
        keyExtractor={item => item._id}
        style={styles.list}
        contentContainerStyle={styles.listContent}
        ListEmptyComponent={
          <View style={styles.emptyContainer}>
            <Text style={styles.emptyText}>
              {isConnected
                ? 'No scripts loaded. Create a new script or load from device.'
                : 'Connect to CAN Creeper to manage scripts.'}
            </Text>
          </View>
        }
      />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0D1117',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#161B22',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  headerTitle: {
    color: '#E6EDF3',
    fontSize: 18,
    fontWeight: '700',
  },
  headerActions: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  newBtn: {
    backgroundColor: '#FF6B00',
    paddingHorizontal: 14,
    paddingVertical: 6,
    borderRadius: 6,
    marginRight: 8,
  },
  newBtnText: {
    color: '#FFFFFF',
    fontSize: 13,
    fontWeight: '700',
  },
  refreshBtn: {
    padding: 6,
  },
  refreshBtnText: {
    fontSize: 18,
  },
  activeBanner: {
    backgroundColor: '#FF6B0015',
    borderWidth: 1,
    borderColor: '#FF6B0040',
    marginHorizontal: 12,
    marginTop: 8,
    borderRadius: 8,
    padding: 12,
  },
  activeTitle: {
    color: '#FF6B00',
    fontSize: 14,
    fontWeight: '700',
    marginBottom: 8,
  },
  progressBarLarge: {
    height: 6,
    backgroundColor: '#21262D',
    borderRadius: 3,
    marginBottom: 6,
    overflow: 'hidden',
  },
  progressFillLarge: {
    height: '100%',
    backgroundColor: '#FF6B00',
    borderRadius: 3,
  },
  activeDetail: {
    color: '#8B949E',
    fontSize: 12,
    fontFamily: 'monospace',
  },
  list: {
    flex: 1,
  },
  listContent: {
    paddingHorizontal: 12,
    paddingTop: 8,
    paddingBottom: 20,
  },
  scriptItem: {
    backgroundColor: '#161B22',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#30363D',
    padding: 12,
    marginBottom: 8,
  },
  scriptItemActive: {
    borderColor: '#FF6B00',
    backgroundColor: '#FF6B0008',
  },
  scriptHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  scriptInfo: {
    flex: 1,
  },
  scriptName: {
    color: '#E6EDF3',
    fontSize: 15,
    fontWeight: '600',
  },
  scriptDesc: {
    color: '#8B949E',
    fontSize: 12,
    marginTop: 2,
  },
  scriptMeta: {
    marginTop: 4,
  },
  metaText: {
    color: '#484F58',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  scriptActions: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  startBtn: {
    backgroundColor: '#238636',
    width: 36,
    height: 36,
    borderRadius: 18,
    alignItems: 'center',
    justifyContent: 'center',
    marginLeft: 6,
  },
  startBtnText: {
    color: '#FFFFFF',
    fontSize: 16,
  },
  stopBtn: {
    backgroundColor: '#DA3633',
    width: 36,
    height: 36,
    borderRadius: 18,
    alignItems: 'center',
    justifyContent: 'center',
    marginLeft: 6,
  },
  stopBtnText: {
    color: '#FFFFFF',
    fontSize: 16,
  },
  editBtn: {
    width: 36,
    height: 36,
    borderRadius: 18,
    alignItems: 'center',
    justifyContent: 'center',
    marginLeft: 4,
  },
  editBtnText: {
    fontSize: 16,
  },
  btnDisabled: {
    opacity: 0.4,
  },
  statusBar: {
    marginTop: 8,
  },
  progressBar: {
    height: 4,
    backgroundColor: '#21262D',
    borderRadius: 2,
    marginBottom: 4,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: '#FF6B00',
    borderRadius: 2,
  },
  statusText: {
    color: '#8B949E',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  builtinBadge: {
    position: 'absolute',
    top: 8,
    right: 8,
    backgroundColor: '#58A6FF20',
    borderRadius: 4,
    paddingHorizontal: 6,
    paddingVertical: 2,
  },
  builtinText: {
    color: '#58A6FF',
    fontSize: 9,
    fontWeight: '700',
  },
  emptyContainer: {
    alignItems: 'center',
    paddingVertical: 40,
  },
  emptyText: {
    color: '#484F58',
    fontSize: 14,
    textAlign: 'center',
  },
  /* Editor styles */
  editorHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    backgroundColor: '#161B22',
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderBottomWidth: 1,
    borderBottomColor: '#30363D',
  },
  editorTitle: {
    color: '#E6EDF3',
    fontSize: 16,
    fontWeight: '700',
  },
  cancelText: {
    color: '#8B949E',
    fontSize: 14,
  },
  saveText: {
    color: '#FF6B00',
    fontSize: 14,
    fontWeight: '700',
  },
  editorNameRow: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#161B22',
  },
  editorLabel: {
    color: '#8B949E',
    fontSize: 13,
    marginRight: 8,
  },
  editorNameInput: {
    flex: 1,
    backgroundColor: '#0D1117',
    borderWidth: 1,
    borderColor: '#30363D',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 6,
    color: '#E6EDF3',
    fontSize: 14,
  },
  editorInput: {
    flex: 1,
    backgroundColor: '#0D1117',
    marginHorizontal: 16,
    marginTop: 8,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#30363D',
    padding: 12,
    color: '#7EE787',
    fontSize: 12,
    fontFamily: 'monospace',
    lineHeight: 18,
  },
  editorHelp: {
    paddingHorizontal: 16,
    paddingVertical: 8,
    backgroundColor: '#161B22',
    borderTopWidth: 1,
    borderTopColor: '#30363D',
  },
  editorHelpText: {
    color: '#484F58',
    fontSize: 10,
    fontFamily: 'monospace',
    marginBottom: 2,
  },
});

export default ScriptsScreen;
