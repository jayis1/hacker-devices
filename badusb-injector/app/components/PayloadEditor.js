/**
 * PHANTOM — DuckyScript Editor Component
 * Syntax-highlighted payload editor with live preview
 *
 * Copyright (C) 2024 Hacker Devices
 * Licensed under MIT
 */

import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TextInput,
  ScrollView,
  TouchableOpacity,
  Alert,
} from 'react-native';

// DuckyScript keywords for syntax highlighting
const DS_KEYWORDS = [
  'STRING', 'DELAY', 'DEFAULT_DELAY', 'DEFAULTDELAY',
  'ENTER', 'TAB', 'ESCAPE', 'ESC', 'SPACE', 'BACKSPACE',
  'CAPSLOCK', 'UP', 'DOWN', 'LEFT', 'RIGHT',
  'GUI', 'WINDOWS', 'SHIFT', 'CTRL', 'CONTROL', 'ALT',
  'CTRL-ALT', 'CTRL-SHIFT',
  'REPEAT', 'HOLD', 'RELEASE',
  'MOUSE_MOVE', 'MOUSE_CLICK', 'MOUSE_SCROLL',
  'CONSUMER', 'REM',
  'STEALTH_ON', 'STEALTH_OFF',
  'LED_R', 'LED_G', 'LED_B',
  'OLED_LINE',
  'RANDOM_DELAY',
  'IF_NETWORK', 'WAIT_FOR_NETWORK',
  'PROFILE',
];

const DS_MODIFIERS = ['CTRL', 'SHIFT', 'ALT', 'GUI', 'WINDOWS', 'CONTROL'];
const DS_VALUES = ['PLAY_PAUSE', 'SCAN_NEXT', 'SCAN_PREV', 'STOP', 'VOL_UP', 'VOL_DOWN', 'MUTE'];

// Line number component
const LineNumber = ({ number, isActive }) => (
  <Text style={[styles.lineNumber, isActive && styles.lineNumberActive]}>
    {number}
  </Text>
);

// Syntax-highlighted line
const HighlightedLine = ({ text }) => {
  const tokens = tokenize(text);
  return (
    <Text style={styles.codeLine}>
      {tokens.map((token, i) => (
        <Text key={i} style={[styles.codeText, token.style]}>
          {token.text}
        </Text>
      ))}
    </Text>
  );
};

// Tokenize a line of DuckyScript
const tokenize = (line) => {
  const tokens = [];
  const trimmed = line.trimStart();
  const leadingSpaces = line.length - trimmed.length;

  if (leadingSpaces > 0) {
    tokens.push({ text: ' '.repeat(leadingSpaces), style: null });
  }

  if (trimmed === '' || trimmed.startsWith('REM')) {
    // Comment line
    tokens.push({ text: trimmed, style: styles.comment });
    return tokens;
  }

  // Split line into words
  const words = trimmed.split(/\s+/);
  let isFirst = true;

  for (const word of words) {
    if (isFirst) {
      // First word is a command
      const upperWord = word.toUpperCase();
      if (DS_KEYWORDS.includes(upperWord)) {
        tokens.push({ text: word, style: styles.keyword });
      } else {
        tokens.push({ text: word, style: styles.command });
      }
      isFirst = false;
    } else {
      // Subsequent words are arguments
      const upperWord = word.toUpperCase();
      if (DS_MODIFIERS.includes(upperWord)) {
        tokens.push({ text: word, style: styles.modifier });
      } else if (DS_VALUES.includes(upperWord)) {
        tokens.push({ text: word, style: styles.value });
      } else if (/^\d+$/.test(word)) {
        tokens.push({ text: word, style: styles.number });
      } else if (/^0x[0-9A-Fa-f]+$/.test(word)) {
        tokens.push({ text: word, style: styles.hexNumber });
      } else {
        tokens.push({ text: word, style: styles.argument });
      }
    }

    // Add space between words (will be handled differently in real rendering)
    tokens.push({ text: ' ', style: null });
  }

  return tokens;
};

// Command reference data
const COMMAND_REFERENCE = [
  { cmd: 'STRING', args: 'text', desc: 'Type a string of text' },
  { cmd: 'DELAY', args: 'ms', desc: 'Wait for specified milliseconds' },
  { cmd: 'DEFAULT_DELAY', args: 'ms', desc: 'Set default delay between commands' },
  { cmd: 'ENTER', args: '', desc: 'Press Enter key' },
  { cmd: 'TAB', args: '', desc: 'Press Tab key' },
  { cmd: 'ESCAPE', args: '', desc: 'Press Escape key' },
  { cmd: 'SPACE', args: '', desc: 'Press Space key' },
  { cmd: 'BACKSPACE', args: '', desc: 'Press Backspace key' },
  { cmd: 'GUI', args: 'key', desc: 'Press GUI (Win/Cmd) + key' },
  { cmd: 'CTRL', args: 'key', desc: 'Press Ctrl + key' },
  { cmd: 'SHIFT', args: 'key', desc: 'Press Shift + key' },
  { cmd: 'ALT', args: 'key', desc: 'Press Alt + key' },
  { cmd: 'CTRL-ALT', args: 'key', desc: 'Press Ctrl+Alt + key' },
  { cmd: 'CTRL-SHIFT', args: 'key', desc: 'Press Ctrl+Shift + key' },
  { cmd: 'UP/DOWN/LEFT/RIGHT', args: '', desc: 'Press arrow key' },
  { cmd: 'F1-F12', args: '', desc: 'Press function key' },
  { cmd: 'REPEAT', args: 'n', desc: 'Repeat previous command n times' },
  { cmd: 'HOLD', args: 'key', desc: 'Hold a key down' },
  { cmd: 'RELEASE', args: '', desc: 'Release all held keys' },
  { cmd: 'MOUSE_MOVE', args: 'dx dy', desc: 'Move mouse by dx, dy pixels' },
  { cmd: 'MOUSE_CLICK', args: 'btn', desc: 'Click mouse button (left/right/middle)' },
  { cmd: 'MOUSE_SCROLL', args: 'n', desc: 'Scroll mouse wheel by n' },
  { cmd: 'CONSUMER', args: 'key', desc: 'Press consumer key (media)' },
  { cmd: 'REM', args: 'text', desc: 'Comment (ignored)' },
  { cmd: 'STEALTH_ON', args: '', desc: 'Switch to HID mode' },
  { cmd: 'STEALTH_OFF', args: '', desc: 'Switch to MSC mode' },
  { cmd: 'RANDOM_DELAY', args: 'min max', desc: 'Random delay between min and max ms' },
  { cmd: 'IF_NETWORK', args: 'ssid', desc: 'Conditional: execute if SSID visible' },
  { cmd: 'WAIT_FOR_NETWORK', args: 'ssid', desc: 'Wait until SSID is visible' },
  { cmd: 'PROFILE', args: 'name', desc: 'Jump to another profile' },
];

const PayloadEditor = ({ payload, onSave, onCancel, connected }) => {
  const [name, setName] = useState(payload?.name || '');
  const [description, setDescription] = useState(payload?.description || '');
  const [script, setScript] = useState(payload?.script || '');
  const [cursorLine, setCursorLine] = useState(1);
  const [cursorCol, setCursorCol] = useState(1);
  const [showReference, setShowReference] = useState(false);
  const [isDirty, setIsDirty] = useState(false);

  useEffect(() => {
    setIsDirty(true);
  }, [name, description, script]);

  const lineCount = script.split('\n').length;

  const handleSave = () => {
    if (!name.trim()) {
      Alert.alert('Validation', 'Payload name is required');
      return;
    }
    if (!script.trim()) {
      Alert.alert('Validation', 'Script cannot be empty');
      return;
    }

    const savedPayload = {
      ...payload,
      name: name.trim(),
      description: description.trim(),
      script,
    };
    onSave(savedPayload);
  };

  const handleInsertCommand = (cmd) => {
    const insertion = cmd.args ? `${cmd.cmd} ` : `${cmd.cmd}\n`;
    setScript(script + insertion);
    setShowReference(false);
  };

  const validateScript = () => {
    const lines = script.split('\n');
    const errors = [];

    for (let i = 0; i < lines.length; i++) {
      const line = lines[i].trim();
      if (!line || line.startsWith('REM')) continue;

      const parts = line.split(/\s+/);
      const command = parts[0].toUpperCase();

      // Check if command is known
      if (!DS_KEYWORDS.includes(command)) {
        // Might be a modifier combo
        const knownCommands = [
          ...DS_KEYWORDS,
          ...DS_MODIFIERS.map((m) => m),
        ];
        const hasModifier = DS_MODIFIERS.includes(command);
        if (!hasModifier && !knownCommands.includes(command)) {
          errors.push(`Line ${i + 1}: Unknown command "${parts[0]}"`);
        }
      }

      // Check DELAY argument
      if (command === 'DELAY') {
        const ms = parseInt(parts[1]);
        if (isNaN(ms) || ms < 0 || ms > 60000) {
          errors.push(`Line ${i + 1}: DELAY must be 0-60000 ms`);
        }
      }

      // Check REPEAT argument
      if (command === 'REPEAT') {
        const count = parseInt(parts[1]);
        if (isNaN(count) || count < 0 || count > 1000) {
          errors.push(`Line ${i + 1}: REPEAT must be 0-1000`);
        }
      }

      // Check STRING has argument
      if (command === 'STRING' && parts.length < 2) {
        errors.push(`Line ${i + 1}: STRING requires text argument`);
      }
    }

    return errors;
  };

  const handleValidate = () => {
    const errors = validateScript();
    if (errors.length === 0) {
      Alert.alert('Validation', '✅ No errors found!', [
        { text: 'OK' },
      ]);
    } else {
      Alert.alert(
        'Validation Errors',
        errors.join('\n'),
        [{ text: 'OK' }]
      );
    }
  };

  return (
    <View style={styles.container}>
      {/* Header */}
      <View style={styles.header}>
        <TouchableOpacity onPress={onCancel}>
          <Text style={styles.cancelButton}>✕ Cancel</Text>
        </TouchableOpacity>
        <Text style={styles.headerTitle}>Edit Payload</Text>
        <TouchableOpacity onPress={handleSave}>
          <Text style={styles.saveButton}>💾 Save</Text>
        </TouchableOpacity>
      </View>

      {/* Metadata */}
      <View style={styles.metaSection}>
        <TextInput
          style={styles.nameInput}
          value={name}
          onChangeText={setName}
          placeholder="Payload name"
          placeholderTextColor="#555"
          autoCapitalize="none"
        />
        <TextInput
          style={styles.descriptionInput}
          value={description}
          onChangeText={setDescription}
          placeholder="Description (optional)"
          placeholderTextColor="#555"
          multiline={false}
        />
      </View>

      {/* Editor Toolbar */}
      <View style={styles.toolbar}>
        <TouchableOpacity
          style={styles.toolButton}
          onPress={() => setShowReference(!showReference)}
        >
          <Text style={styles.toolButtonText}>📖 Commands</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.toolButton} onPress={handleValidate}>
          <Text style={styles.toolButtonText}>✅ Validate</Text>
        </TouchableOpacity>
        <View style={styles.cursorInfo}>
          <Text style={styles.cursorInfoText}>
            Ln {cursorLine}, Col {cursorCol}
          </Text>
        </View>
      </View>

      {/* Command Reference */}
      {showReference && (
        <ScrollView style={styles.referencePanel} nestedScrollEnabled>
          {COMMAND_REFERENCE.map((ref, i) => (
            <TouchableOpacity
              key={i}
              style={styles.referenceItem}
              onPress={() => handleInsertCommand(ref)}
            >
              <Text style={styles.refCommand}>{ref.cmd}</Text>
              {ref.args && (
                <Text style={styles.refArgs}>{ref.args}</Text>
              )}
              <Text style={styles.refDesc}>{ref.desc}</Text>
            </TouchableOpacity>
          ))}
        </ScrollView>
      )}

      {/* Script Editor */}
      <View style={styles.editorContainer}>
        {/* Line numbers */}
        <ScrollView style={styles.lineNumbers}>
          {Array.from({ length: lineCount }, (_, i) => (
            <LineNumber
              key={i}
              number={i + 1}
              isActive={i + 1 === cursorLine}
            />
          ))}
        </ScrollView>

        {/* Code area */}
        <TextInput
          style={styles.codeInput}
          value={script}
          onChangeText={setScript}
          multiline
          autoCapitalize="none"
          autoCorrect={false}
          spellCheck={false}
          fontFamily="monospace"
          placeholder="Enter DuckyScript here..."
          placeholderTextColor="#333"
          scrollEnabled
        />
      </View>

      {/* Script Preview (syntax highlighted) */}
      <View style={styles.previewHeader}>
        <Text style={styles.previewTitle}>PREVIEW</Text>
      </View>
      <ScrollView style={styles.previewPanel} nestedScrollEnabled>
        {script.split('\n').map((line, i) => (
          <View key={i} style={styles.previewLine}>
            <Text style={styles.previewLineNumber}>{i + 1}</Text>
            <HighlightedLine text={line} />
          </View>
        ))}
      </ScrollView>

      {/* Action Buttons */}
      <View style={styles.actionBar}>
        <TouchableOpacity
          style={[styles.actionBtn, styles.saveBtn]}
          onPress={handleSave}
        >
          <Text style={styles.actionBtnText}>Save Payload</Text>
        </TouchableOpacity>
        {connected && (
          <TouchableOpacity
            style={[styles.actionBtn, styles.uploadBtn]}
            onPress={() => onSave({ ...payload, name, description, script })}
          >
            <Text style={styles.actionBtnText}>Upload to Device</Text>
          </TouchableOpacity>
        )}
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0f0f23',
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  headerTitle: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 16,
    fontWeight: 'bold',
  },
  cancelButton: {
    color: '#e74c3c',
    fontFamily: 'monospace',
    fontSize: 14,
  },
  saveButton: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
  metaSection: {
    padding: 16,
    backgroundColor: '#1a1a2e',
  },
  nameInput: {
    backgroundColor: '#0f0f23',
    borderRadius: 8,
    padding: 12,
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 16,
    borderWidth: 1,
    borderColor: '#2d2d44',
    marginBottom: 8,
  },
  descriptionInput: {
    backgroundColor: '#0f0f23',
    borderRadius: 8,
    padding: 12,
    color: '#fff',
    fontFamily: 'monospace',
    fontSize: 13,
    borderWidth: 1,
    borderColor: '#2d2d44',
  },
  toolbar: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 8,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  toolButton: {
    backgroundColor: '#2d2d44',
    borderRadius: 6,
    paddingHorizontal: 10,
    paddingVertical: 4,
    marginRight: 8,
  },
  toolButtonText: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 11,
  },
  cursorInfo: {
    marginLeft: 'auto',
  },
  cursorInfoText: {
    color: '#666',
    fontFamily: 'monospace',
    fontSize: 10,
  },
  referencePanel: {
    maxHeight: 150,
    backgroundColor: '#1a1a2e',
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  referenceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 6,
    borderBottomWidth: 1,
    borderBottomColor: '#2d2d44',
  },
  refCommand: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 11,
    fontWeight: 'bold',
    width: 120,
  },
  refArgs: {
    color: '#f39c12',
    fontFamily: 'monospace',
    fontSize: 11,
    width: 60,
  },
  refDesc: {
    color: '#888',
    fontFamily: 'monospace',
    fontSize: 11,
    flex: 1,
  },
  editorContainer: {
    flexDirection: 'row',
    flex: 1,
    backgroundColor: '#0a0a15',
  },
  lineNumbers: {
    width: 40,
    backgroundColor: '#0a0a15',
    paddingVertical: 8,
  },
  lineNumber: {
    color: '#333',
    fontFamily: 'monospace',
    fontSize: 12,
    lineHeight: 20,
    textAlign: 'right',
    paddingRight: 8,
  },
  lineNumberActive: {
    color: '#00ff41',
  },
  codeInput: {
    flex: 1,
    backgroundColor: '#0a0a15',
    color: '#e0e0e0',
    fontFamily: 'monospace',
    fontSize: 13,
    lineHeight: 20,
    padding: 8,
    textAlignVertical: 'top',
  },
  codeLine: {
    lineHeight: 20,
  },
  codeText: {
    fontFamily: 'monospace',
    fontSize: 13,
  },
  previewHeader: {
    backgroundColor: '#1a1a2e',
    paddingVertical: 6,
    paddingHorizontal: 12,
    borderTopWidth: 1,
    borderBottomWidth: 1,
    borderColor: '#2d2d44',
  },
  previewTitle: {
    color: '#00ff41',
    fontFamily: 'monospace',
    fontSize: 10,
    letterSpacing: 1,
  },
  previewPanel: {
    maxHeight: 120,
    backgroundColor: '#0a0a15',
    padding: 8,
  },
  previewLine: {
    flexDirection: 'row',
    lineHeight: 18,
  },
  previewLineNumber: {
    color: '#333',
    fontFamily: 'monospace',
    fontSize: 11,
    width: 30,
    textAlign: 'right',
    marginRight: 8,
  },
  comment: { color: '#6a9955' },
  keyword: { color: '#00ff41', fontWeight: 'bold' },
  command: { color: '#569cd6' },
  modifier: { color: '#c586c0' },
  value: { color: '#4ec9b0' },
  number: { color: '#b5cea8' },
  hexNumber: { color: '#d7ba7d' },
  argument: { color: '#ce9178' },
  actionBar: {
    flexDirection: 'row',
    padding: 12,
    backgroundColor: '#1a1a2e',
    borderTopWidth: 1,
    borderTopColor: '#2d2d44',
  },
  actionBtn: {
    flex: 1,
    borderRadius: 8,
    padding: 12,
    alignItems: 'center',
    marginRight: 8,
  },
  saveBtn: {
    backgroundColor: '#00ff41',
  },
  uploadBtn: {
    backgroundColor: '#3498db',
  },
  actionBtnText: {
    color: '#0f0f23',
    fontFamily: 'monospace',
    fontSize: 14,
    fontWeight: 'bold',
  },
});

export default PayloadEditor;