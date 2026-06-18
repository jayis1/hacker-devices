/*
 * components/SafetyInterlock.js — Safety interlock component for spoofing controls
 *
 * Displays a multi-step safety confirmation dialog before enabling transmission.
 * Requires: (1) safety switch LIVE, (2) arm command, (3) start command.
 *
 * Author: jayis1
 * License: Proprietary — Authorized Security Research Use Only
 */

import React, { useState, useEffect } from 'react';
import {
  View, Text, StyleSheet, Modal, TouchableOpacity,
  Alert, Vibration,
} from 'react-native';

export default function SafetyInterlock({ visible, onConfirm, onCancel, steps = [] }) {
  const [stepIndex, setStepIndex] = useState(0);
  const [holdProgress, setHoldProgress] = useState(0);

  useEffect(() => {
    if (!visible) {
      setStepIndex(0);
      setHoldProgress(0);
    }
  }, [visible]);

  const handleStep = (index) => {
    if (index < steps.length - 1) {
      setStepIndex(index + 1);
    } else {
      Vibration.vibrate([100, 50, 100, 50, 200]);
      onConfirm && onConfirm();
    }
  };

  return (
    <Modal visible={visible} transparent={true} animationType="slide">
      <View style={styles.overlay}>
        <View style={styles.dialog}>
          <Text style={styles.title}>⚠️ SAFETY INTERLOCK</Text>
          <Text style={styles.warning}>
            GNSS signal spoofing is potentially dangerous and illegal
            outside authorized RF-shielded environments. Confirm all steps
            to proceed.
          </Text>

          {steps.map((step, i) => (
            <TouchableOpacity
              key={i}
              style={[
                styles.stepButton,
                i === stepIndex ? styles.stepActive : styles.stepInactive,
                i < stepIndex ? styles.stepDone : {},
              ]}
              onPress={() => i === stepIndex && handleStep(i)}
              disabled={i !== stepIndex}
            >
              <Text style={styles.stepText}>
                {i < stepIndex ? '✓ ' : (i === stepIndex ? '→ ' : '○ ')}
                {step}
              </Text>
            </TouchableOpacity>
          ))}

          <TouchableOpacity style={styles.cancelButton} onPress={onCancel}>
            <Text style={styles.cancelText}>CANCEL</Text>
          </TouchableOpacity>
        </View>
      </View>
    </Modal>
  );
}

const styles = StyleSheet.create({
  overlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.85)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  dialog: {
    width: '90%',
    backgroundColor: '#1a1a2e',
    borderRadius: 12,
    padding: 20,
    borderWidth: 2,
    borderColor: '#ef4444',
  },
  title: {
    color: '#ef4444',
    fontSize: 18,
    fontWeight: 'bold',
    textAlign: 'center',
    marginBottom: 10,
  },
  warning: {
    color: '#fbbf24',
    fontSize: 12,
    textAlign: 'center',
    marginBottom: 20,
    lineHeight: 18,
  },
  stepButton: {
    padding: 12,
    borderRadius: 8,
    marginBottom: 8,
  },
  stepActive: {
    backgroundColor: '#ef4444',
  },
  stepInactive: {
    backgroundColor: '#2a2a3e',
  },
  stepDone: {
    backgroundColor: '#22c55e',
  },
  stepText: {
    color: '#fff',
    fontSize: 14,
    fontFamily: 'monospace',
  },
  cancelButton: {
    marginTop: 16,
    padding: 12,
    alignItems: 'center',
  },
  cancelText: {
    color: '#888',
    fontSize: 14,
  },
});