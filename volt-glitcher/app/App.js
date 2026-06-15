import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import ControlScreen from './screens/ControlScreen';
import AnalysisScreen from './screens/AnalysisScreen';

const Stack = createStackNavigator();

export default function App() {
  return (
    <NavigationContainer>
      <Stack.Navigator initialRouteName="Control">
        <Stack.Screen name="Control" component={ControlScreen} options={{ title: 'Volt-Glitcher Control' }} />
        <Stack.Screen name="Analysis" component={AnalysisScreen} options={{ title: 'Power Trace Analysis' }} />
      </Stack.Navigator>
    </NavigationContainer>
  );
}
