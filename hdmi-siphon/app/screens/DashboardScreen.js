/**
 * DashboardScreen.js — Main device status dashboard
 * Author: jayis1
 */

import React from 'react';
import {
  View,
  Text,
  StyleSheet,
  ScrollView,
  RefreshControl,
  TouchableOpacity,
} from 'react-native';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import StatusCard from '../components/StatusCard';

const DashboardScreen = ({status, connected, protocol}) => {
  const [refreshing, setRefreshing] = React.useState(false);

  const onRefresh = () => {
    setRefreshing(true);
    if (protocol) {
      protocol.getStatus(() => setRefreshing(false));
    } else {
      setRefreshing(false);
    }
  };

  const resolution = status?.resolution || '--x--';
  const refreshRate = status?.refresh_x100
    ? `${(status.refresh_x100 / 100).toFixed(2)} Hz`
    : '-- Hz';
  const pclk = status?.pclk_khz ? `${(status.pclk_khz / 1000).toFixed(1)} MHz` : '-- MHz';
  const batteryPct = status?.battery_pct ?? '--';
  const batteryMv = status?.battery_mv ?? '--';
  const frameCount = status?.frame_count ?? 0;
  const sdFree = status?.sd_free
    ? `${(status.sd_free / (1024 * 1024)).toFixed(0)} MB`
    : 'N/A';
  const cecCount = status?.cec_msg_count ?? 0;

  const isConnected = connected && status?.fpga_ready;

  return (
    <ScrollView
      style={styles.container}
      refreshControl={
        <RefreshControl refreshing={refreshing} onRefresh={onRefresh} />
      }>
      {/* Connection Status Banner */}
      <View style={[styles.banner, isConnected ? styles.bannerConnected : styles.bannerDisconnected]}>
        <Icon
          name={isConnected ? 'check-circle' : 'alert-circle'}
          size={20}
          color="#FFF"
        />
        <Text style={styles.bannerText}>
          {isConnected ? 'Device Connected' : 'Disconnected'}
        </Text>
      </View>

      {/* Video Status */}
      <Text style={styles.sectionTitle}>Video Signal</Text>
      <View style={styles.statusRow}>
        <StatusCard
          icon="monitor"
          label="Resolution"
          value={resolution}
          color="#42A5F5"
        />
        <StatusCard
          icon="refresh"
          label="Refresh"
          value={refreshRate}
          color="#66BB6A"
        />
        <StatusCard
          icon="clock-outline"
          label="Pixel Clock"
          value={pclk}
          color="#FFA726"
        />
      </View>

      {/* Signal Status */}
      <View style={styles.statusRow}>
        <StatusCard
          icon={status?.hdmi_locked ? 'lock-open-variant' : 'lock'}
          label="HDMI Lock"
          value={status?.hdmi_locked ? 'LOCKED' : 'UNLOCKED'}
          color={status?.hdmi_locked ? '#66BB6A' : '#EF5350'}
        />
        <StatusCard
          icon={status?.display_connected ? 'monitor' : 'monitor-off'}
          label="Display"
          value={status?.display_connected ? 'CONNECTED' : 'NOT DETECTED'}
          color={status?.display_connected ? '#66BB6A' : '#EF5350'}
        />
        <StatusCard
          icon={status?.hdcp_active ? 'shield-lock' : 'shield-off'}
          label="HDCP"
          value={status?.hdcp_active ? 'ACTIVE' : 'INACTIVE'}
          color={status?.hdcp_active ? '#FFA726' : '#78909C'}
        />
      </View>

      {/* System Status */}
      <Text style={styles.sectionTitle}>System</Text>
      <View style={styles.statusRow}>
        <StatusCard
          icon="battery"
          label="Battery"
          value={`${batteryPct}% (${batteryMv} mV)`}
          color={
            batteryPct > 50
              ? '#66BB6A'
              : batteryPct > 20
              ? '#FFA726'
              : '#EF5350'
          }
        />
        <StatusCard
          icon="camera"
          label="Frames"
          value={String(frameCount)}
          color="#AB47BC"
        />
        <StatusCard
          icon="sd"
          label="Storage"
          value={sdFree}
          color="#26C6DA"
        />
      </View>

      {/* CEC & EDID */}
      <View style={styles.statusRow}>
        <StatusCard
          icon="remote"
          label="CEC Messages"
          value={String(cecCount)}
          color="#8D6E63"
        />
        <StatusCard
          icon={status?.edid_valid ? 'card-bulleted-outline' : 'card-bulleted-off'}
          label="EDID"
          value={status?.edid_valid ? 'VALID' : 'INVALID'}
          color={status?.edid_valid ? '#66BB6A' : '#EF5350'}
        />
        <StatusCard
          icon="chip"
          label="FPGA"
          value={status?.fpga_ready ? 'READY' : 'NOT READY'}
          color={status?.fpga_ready ? '#66BB6A' : '#EF5350'}
        />
      </View>

      {/* Quick Actions */}
      <Text style={styles.sectionTitle}>Quick Actions</Text>
      <View style={styles.actionRow}>
        <TouchableOpacity
          style={[styles.actionButton, {backgroundColor: '#E53935'}]}
          onPress={() => protocol?.captureFrame()}>
          <Icon name="camera" size={24} color="#FFF" />
          <Text style={styles.actionText}>Capture</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, {backgroundColor: '#1E88E5'}]}
          onPress={() => protocol?.sendCommand('status')}>
          <Icon name="refresh" size={24} color="#FFF" />
          <Text style={styles.actionText}>Refresh</Text>
        </TouchableOpacity>
        <TouchableOpacity
          style={[styles.actionButton, {backgroundColor: '#43A047'}]}
          onPress={() => protocol?.setMode('passthrough')}>
          <Icon name="eye" size={24} color="#FFF" />
          <Text style={styles.actionText}>Passthru</Text>
        </TouchableOpacity>
      </View>

      {/* Author credit */}
      <View style={styles.footer}>
        <Text style={styles.footerText}>HDMI-Siphon v1.0.0</Text>
        <Text style={styles.footerSubtext}>Author: jayis1</Text>
        <Text style={styles.footerLegal}>
          Authorized Security Research Only
        </Text>
      </View>
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#121212',
    padding: 12,
  },
  banner: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 10,
    borderRadius: 8,
    marginBottom: 12,
  },
  bannerConnected: {
    backgroundColor: '#2E7D32',
  },
  bannerDisconnected: {
    backgroundColor: '#C62828',
  },
  bannerText: {
    color: '#FFF',
    fontSize: 14,
    fontWeight: '600',
    marginLeft: 8,
  },
  sectionTitle: {
    color: '#FFF',
    fontSize: 16,
    fontWeight: '700',
    marginTop: 8,
    marginBottom: 8,
    paddingLeft: 4,
  },
  statusRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  actionRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 4,
  },
  actionButton: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    padding: 12,
    borderRadius: 8,
    marginHorizontal: 4,
  },
  actionText: {
    color: '#FFF',
    fontSize: 13,
    fontWeight: '600',
    marginLeft: 6,
  },
  footer: {
    alignItems: 'center',
    marginTop: 24,
    paddingBottom: 20,
  },
  footerText: {
    color: '#78909C',
    fontSize: 12,
  },
  footerSubtext: {
    color: '#78909C',
    fontSize: 11,
  },
  footerLegal: {
    color: '#EF5350',
    fontSize: 10,
    marginTop: 4,
  },
});

export default DashboardScreen;
