/**
 * Balance Display Component
 */

import React from 'react';
import {View, Text, StyleSheet} from 'react-native';
import {Card} from '../common';

interface BalanceCardProps {
  balance: string;
  currency?: string;
  usdValue?: string;
}

export const BalanceCard: React.FC<BalanceCardProps> = ({
  balance,
  currency = 'DRACHMA',
  usdValue,
}) => {
  return (
    <Card style={styles.container}>
      <Text style={styles.label}>Balance</Text>
      <Text style={styles.balance}>
        {balance} {currency}
      </Text>
      {usdValue && <Text style={styles.usdValue}>≈ ${usdValue} USD</Text>}
    </Card>
  );
};

const styles = StyleSheet.create({
  container: {
    alignItems: 'center',
  },
  label: {
    fontSize: 14,
    color: '#666',
    marginBottom: 8,
  },
  balance: {
    fontSize: 32,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 4,
  },
  usdValue: {
    fontSize: 16,
    color: '#999',
  },
});
