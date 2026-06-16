/**
 * TrafficChart.js — Real-Time Traffic Visualization
 * Author: jayis1
 */

import React from 'react';
import { View, Text, Dimensions, StyleSheet } from 'react-native';
import { LineChart } from 'react-native-chart-kit';

const screenWidth = Dimensions.get('window').width - 48;

const TrafficChart = ({ data }) => {
  if (!data || !data.labels || data.labels.length < 2) {
    return (
      <View style={styles.placeholder}>
        <Text style={styles.placeholderText}>Waiting for traffic data...</Text>
      </View>
    );
  }

  return (
    <LineChart
      data={{
        labels: data.labels.map((_, i) => i % 5 === 0 ? data.labels[i] : ''),
        datasets: [
          {
            data: data.reads.length >= 2 ? data.reads : [0, 0],
            color: (opacity) => `rgba(68, 136, 255, ${opacity})`,
            strokeWidth: 2,
          },
          {
            data: data.writes.length >= 2 ? data.writes : [0, 0],
            color: (opacity) => `rgba(255, 68, 68, ${opacity})`,
            strokeWidth: 2,
          },
        ],
        legend: ['Reads', 'Writes'],
      }}
      width={screenWidth}
      height={180}
      yAxisSuffix=""
      yAxisInterval={1}
      chartConfig={{
        backgroundColor: '#12122a',
        backgroundGradientFrom: '#12122a',
        backgroundGradientTo: '#0a0a1a',
        decimalCount: 0,
        color: (opacity) => `rgba(0, 255, 136, ${opacity})`,
        labelColor: (opacity) => `rgba(170, 170, 170, ${opacity})`,
        propsForDots: {
          r: '3',
          strokeWidth: '1',
          stroke: '#00ff88',
        },
        propsForBackgroundLines: {
          strokeDasharray: '4,4',
          stroke: '#2a2a4a',
        },
      }}
      bezier
      style={styles.chart}
    />
  );
};

const styles = StyleSheet.create({
  chart: {
    borderRadius: 8,
  },
  placeholder: {
    height: 180,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#0a0a1a',
    borderRadius: 8,
  },
  placeholderText: {
    color: '#555',
    fontSize: 13,
  },
});

export default TrafficChart;
