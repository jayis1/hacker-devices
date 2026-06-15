import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { VictoryLine, VictoryChart, VictoryTheme } from 'victory-native';

const AnalysisScreen = () => {
  const dummyData = [
    { x: 0, y: 3.3 }, { x: 10, y: 3.2 }, { x: 20, y: 0.1 }, { x: 25, y: 0.1 }, 
    { x: 30, y: 3.1 }, { x: 40, y: 3.3 }
  ];

  return (
    <View style={styles.container}>
      <Text style={styles.title}>Live Power Trace</Text>
      <VictoryChart theme={VictoryTheme.material} height={300}>
        <VictoryLine
          style={{ data: { stroke: "#c43a31" } }}
          data={dummyData}
        />
      </VictoryChart>
    </View>
  );
};

const styles = StyleSheet.create({
  container: { flex: 1, padding: 10, backgroundColor: '#fff' },
  title: { fontSize: 20, fontWeight: 'bold', textAlign: 'center' }
});

export default AnalysisScreen;
