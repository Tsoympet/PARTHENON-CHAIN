/**
 * App Navigator
 */

import React from 'react';
import {createStackNavigator} from '@react-navigation/stack';
import {createBottomTabNavigator} from '@react-navigation/bottom-tabs';
import {Ionicons} from '@expo/vector-icons';
import {
  HomeScreen,
  WalletScreen,
  SendScreen,
  ReceiveScreen,
  TransactionsScreen,
  SettingsScreen,
  NFTScreen,
} from '@screens';

const Tab = createBottomTabNavigator();
const Stack = createStackNavigator();

function MainTabs() {
  return (
    <Tab.Navigator
      screenOptions={({route}) => ({
        headerShown: false,
        tabBarIcon: ({color, size}) => {
          const iconMap: Record<string, keyof typeof Ionicons.glyphMap> = {
            Home: 'home-outline',
            Wallet: 'wallet-outline',
            Send: 'send-outline',
            Receive: 'download-outline',
            NFT: 'images-outline',
            Transactions: 'swap-horizontal-outline',
            Settings: 'settings-outline',
          };

          return <Ionicons name={iconMap[route.name]} size={size} color={color} />;
        },
      })}>
      <Tab.Screen name="Home" component={HomeScreen} />
      <Tab.Screen name="Wallet" component={WalletScreen} />
      <Tab.Screen name="Send" component={SendScreen} />
      <Tab.Screen name="Receive" component={ReceiveScreen} />
      <Tab.Screen name="NFT" component={NFTScreen} />
      <Tab.Screen name="Transactions" component={TransactionsScreen} />
      <Tab.Screen name="Settings" component={SettingsScreen} />
    </Tab.Navigator>
  );
}

export function AppNavigator() {
  return (
    <Stack.Navigator>
      <Stack.Screen 
        name="MainTabs" 
        component={MainTabs}
        options={{headerShown: false}}
      />
    </Stack.Navigator>
  );
}
