/**
 * AppNavigator.js — SATA Phantom Navigation
 * Author: jayis1
 *
 * Defines the bottom tab navigation structure with all screen routes.
 */

import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { createStackNavigator } from '@react-navigation/stack';
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';

import DiscoveryScreen from '../screens/DiscoveryScreen';
import DashboardScreen from '../screens/DashboardScreen';
import RulesScreen from '../screens/RulesScreen';
import InjectorScreen from '../screens/InjectorScreen';
import ExfilScreen from '../screens/ExfilScreen';
import ConsoleScreen from '../screens/ConsoleScreen';
import SettingsScreen from '../screens/SettingsScreen';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

const screenOptions = {
  headerStyle: {
    backgroundColor: '#12122a',
    borderBottomColor: '#2a2a4a',
    borderBottomWidth: 1,
  },
  headerTintColor: '#00ff88',
  headerTitleStyle: {
    fontWeight: 'bold',
    fontSize: 16,
  },
};

const DiscoveryStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Discovery" component={DiscoveryScreen} options={{ title: 'SATA Phantom' }} />
  </Stack.Navigator>
);

const DashboardStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Dashboard" component={DashboardScreen} options={{ title: 'Live Traffic' }} />
  </Stack.Navigator>
);

const RulesStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Rules" component={RulesScreen} options={{ title: 'Rules Engine' }} />
  </Stack.Navigator>
);

const InjectorStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Injector" component={InjectorScreen} options={{ title: 'Frame Injector' }} />
  </Stack.Navigator>
);

const ExfilStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Exfil" component={ExfilScreen} options={{ title: 'Exfiltration' }} />
  </Stack.Navigator>
);

const MoreStack = () => (
  <Stack.Navigator screenOptions={screenOptions}>
    <Stack.Screen name="Console" component={ConsoleScreen} options={{ title: 'Console' }} />
    <Stack.Screen name="Settings" component={SettingsScreen} options={{ title: 'Settings' }} />
  </Stack.Navigator>
);

const AppNavigator = () => {
  return (
    <Tab.Navigator
      screenOptions={({ route }) => ({
        tabBarIcon: ({ focused, color, size }) => {
          let iconName;
          switch (route.name) {
            case 'Discovery': iconName = 'radar'; break;
            case 'Dashboard': iconName = 'chart-line'; break;
            case 'Rules': iconName = 'filter-variant'; break;
            case 'Injector': iconName = 'inject'; break;
            case 'Exfil': iconName = 'upload-network'; break;
            case 'More': iconName = 'dots-horizontal'; break;
            default: iconName = 'circle';
          }
          return <Icon name={iconName} size={size} color={color} />;
        },
        tabBarActiveTintColor: '#00ff88',
        tabBarInactiveTintColor: '#555',
        tabBarStyle: {
          backgroundColor: '#12122a',
          borderTopColor: '#2a2a4a',
          borderTopWidth: 1,
          paddingBottom: 4,
          height: 60,
        },
        headerShown: false,
      })}
    >
      <Tab.Screen name="Discovery" component={DiscoveryStack} options={{ title: 'Discover' }} />
      <Tab.Screen name="Dashboard" component={DashboardStack} options={{ title: 'Traffic' }} />
      <Tab.Screen name="Rules" component={RulesStack} options={{ title: 'Rules' }} />
      <Tab.Screen name="Injector" component={InjectorStack} options={{ title: 'Inject' }} />
      <Tab.Screen name="Exfil" component={ExfilStack} options={{ title: 'Exfil' }} />
      <Tab.Screen name="More" component={MoreStack} options={{ title: 'More' }} />
    </Tab.Navigator>
  );
};

export default AppNavigator;
