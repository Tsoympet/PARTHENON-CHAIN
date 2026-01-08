/**
 * Transaction List Item Component
 */

import React from 'react';
import {View, Text, StyleSheet, TouchableOpacity} from 'react-native';

export interface Transaction {
  id: string;
  type: 'send' | 'receive';
  amount: string;
  address: string;
  timestamp: number;
  status: 'pending' | 'confirmed' | 'failed';
}

interface TransactionItemProps {
  transaction: Transaction;
  onPress?: () => void;
}

export const TransactionItem: React.FC<TransactionItemProps> = ({
  transaction,
  onPress,
}) => {
  const isSend = transaction.type === 'send';
  const statusColor =
    transaction.status === 'confirmed'
      ? '#28A745'
      : transaction.status === 'pending'
      ? '#FFC107'
      : '#DC3545';

  return (
    <TouchableOpacity
      style={styles.container}
      onPress={onPress}
      activeOpacity={0.7}>
      <View style={styles.left}>
        <Text style={styles.type}>{isSend ? '↑ Sent' : '↓ Received'}</Text>
        <Text style={styles.address}>
          {transaction.address.slice(0, 10)}...
        </Text>
      </View>
      <View style={styles.right}>
        <Text style={[styles.amount, isSend && styles.sendAmount]}>
          {isSend ? '-' : '+'}
          {transaction.amount}
        </Text>
        <Text style={[styles.status, {color: statusColor}]}>
          {transaction.status}
        </Text>
      </View>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#FFF',
    borderBottomWidth: 1,
    borderBottomColor: '#EEE',
  },
  left: {
    flex: 1,
  },
  right: {
    alignItems: 'flex-end',
  },
  type: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 4,
  },
  address: {
    fontSize: 12,
    color: '#999',
    fontFamily: 'Courier',
  },
  amount: {
    fontSize: 16,
    fontWeight: '600',
    color: '#28A745',
    marginBottom: 4,
  },
  sendAmount: {
    color: '#DC3545',
  },
  status: {
    fontSize: 12,
    textTransform: 'capitalize',
  },
});
