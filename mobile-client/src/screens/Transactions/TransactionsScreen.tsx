/**
 * Transactions Screen
 */

import React from 'react';
import {FlatList, SafeAreaView, StyleSheet, Text, View} from 'react-native';
import {useSelector} from 'react-redux';
import {TransactionItem} from '@components';
import {RootState} from '@store';
import {Transaction} from '@types';

export const TransactionsScreen: React.FC = () => {
  const transactions = useSelector((state: RootState) => state.wallet.transactions);

  const handleTransactionPress = (transaction: Transaction) => {
    console.log('Transaction pressed:', transaction.id);
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Transactions</Text>
      </View>
      <FlatList
        data={transactions}
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
