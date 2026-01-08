/**
 * Transactions Screen
 */

import React from 'react';
import {View, Text, StyleSheet, FlatList, SafeAreaView} from 'react-native';
import {TransactionItem} from '@components';
import {Transaction} from '@types';

// Mock data - replace with Redux state
const mockTransactions: Transaction[] = [
  {
    id: '1',
    type: 'receive',
    from: 'drm1234567890abcdef',
    to: 'drm0000000000000000',
    amount: '100.00',
    timestamp: Date.now() - 3600000,
    status: 'confirmed',
  },
  {
    id: '2',
    type: 'send',
    from: 'drm0000000000000000',
    to: 'drm0987654321fedcba',
    amount: '50.00',
    timestamp: Date.now() - 7200000,
    status: 'confirmed',
  },
];

export const TransactionsScreen: React.FC = () => {
  const handleTransactionPress = (transaction: Transaction) => {
    // TODO: Navigate to transaction details
    console.log('Transaction pressed:', transaction.id);
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Transactions</Text>
      </View>
      <FlatList
        data={mockTransactions}
        renderItem={({item}) => (
          <TransactionItem
            transaction={item}
            onPress={() => handleTransactionPress(item)}
          />
        )}
        keyExtractor={item => item.id}
        ListEmptyComponent={
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No transactions yet</Text>
          </View>
        }
      />
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#F5F5F5',
  },
  header: {
    padding: 16,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#EEE',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
  },
  empty: {
    padding: 48,
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 16,
    color: '#999',
  },
});
