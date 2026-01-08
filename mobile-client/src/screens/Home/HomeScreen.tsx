/**
 * Home Screen
 */

import React from 'react';
import {View, Text, StyleSheet, ScrollView, SafeAreaView} from 'react-native';
import {useSelector} from 'react-redux';
import {RootState} from '@store';
import {BalanceCard, Button} from '@components';

export const HomeScreen: React.FC = () => {
  const {balance, address} = useSelector((state: RootState) => state.wallet);

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Drachma Wallet</Text>
        
        <BalanceCard 
          balance={balance || '0.00'} 
          currency="DRACHMA"
        />

        <View style={styles.actions}>
          <Button 
            title="Send" 
            onPress={() => {}} 
            style={styles.actionButton}
          />
          <Button 
            title="Receive" 
            onPress={() => {}} 
            style={styles.actionButton}
            variant="outline"
          />
        </View>

        {address && (
          <View style={styles.addressSection}>
            <Text style={styles.sectionTitle}>Your Address</Text>
            <Text style={styles.address}>{address}</Text>
          </View>
        )}
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
    textAlign: 'center',
  },
  actions: {
    flexDirection: 'row',
    gap: 12,
    marginVertical: 16,
  },
  actionButton: {
    flex: 1,
  },
  addressSection: {
    marginTop: 24,
  },
  sectionTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 8,
  },
  address: {
    fontSize: 12,
    fontFamily: 'Courier',
    color: '#666',
    padding: 12,
    backgroundColor: '#FFF',
    borderRadius: 8,
  },
});
