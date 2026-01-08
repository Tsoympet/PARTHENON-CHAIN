/**
 * Settings Screen
 */

import React from 'react';
import {View, Text, StyleSheet, ScrollView, SafeAreaView, TouchableOpacity} from 'react-native';
import {Card, Button} from '@components';

interface SettingItemProps {
  title: string;
  subtitle?: string;
  onPress: () => void;
}

const SettingItem: React.FC<SettingItemProps> = ({title, subtitle, onPress}) => (
  <TouchableOpacity onPress={onPress} activeOpacity={0.7}>
    <Card>
      <Text style={styles.itemTitle}>{title}</Text>
      {subtitle && <Text style={styles.itemSubtitle}>{subtitle}</Text>}
    </Card>
  </TouchableOpacity>
);

export const SettingsScreen: React.FC = () => {
  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Settings</Text>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Wallet</Text>
          <SettingItem
            title="Backup Wallet"
            subtitle="View recovery phrase"
            onPress={() => console.log('Backup wallet')}
          />
          <SettingItem
            title="Security"
            subtitle="Biometrics, PIN"
            onPress={() => console.log('Security settings')}
          />
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>Network</Text>
          <SettingItem
            title="Node Settings"
            subtitle="Configure RPC endpoint"
            onPress={() => console.log('Node settings')}
          />
          <SettingItem
            title="Mining"
            subtitle="Mobile mining settings"
            onPress={() => console.log('Mining settings')}
          />
        </View>

        <View style={styles.section}>
          <Text style={styles.sectionTitle}>About</Text>
          <SettingItem
            title="Version"
            subtitle="0.1.0"
            onPress={() => console.log('Version info')}
          />
          <SettingItem
            title="Terms of Service"
            onPress={() => console.log('Terms')}
          />
        </View>

        <Button
          title="Lock Wallet"
          onPress={() => console.log('Lock wallet')}
          variant="outline"
          style={styles.lockButton}
        />
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  content: {
    padding: 16,
  },
  title: {
    fontSize: 28,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 24,
  },
  section: {
    marginBottom: 24,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#666',
    marginBottom: 12,
    marginLeft: 4,
  },
  itemTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
  },
  itemSubtitle: {
    fontSize: 14,
    color: '#666',
    marginTop: 4,
  },
  lockButton: {
    marginTop: 24,
  },
});
