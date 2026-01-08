/**
 * Address Display Component with QR Code
 */

import React from 'react';
import {View, Text, StyleSheet, TouchableOpacity} from 'react-native';
import {Card} from '../common';

interface AddressDisplayProps {
  address: string;
  onCopy?: () => void;
}

export const AddressDisplay: React.FC<AddressDisplayProps> = ({
  address,
  onCopy,
}) => {
  const truncatedAddress = `${address.slice(0, 10)}...${address.slice(-8)}`;

  return (
    <Card>
      <Text style={styles.label}>Your Address</Text>
      <TouchableOpacity onPress={onCopy} activeOpacity={0.7}>
        <Text style={styles.address}>{truncatedAddress}</Text>
        <Text style={styles.fullAddress}>{address}</Text>
      </TouchableOpacity>
      {onCopy && <Text style={styles.hint}>Tap to copy</Text>}
    </Card>
  );
};

const styles = StyleSheet.create({
  label: {
    fontSize: 14,
    color: '#666',
    marginBottom: 8,
  },
  address: {
    fontSize: 18,
    fontWeight: '600',
    color: '#007AFF',
    marginBottom: 4,
  },
  fullAddress: {
    fontSize: 12,
    color: '#999',
    fontFamily: 'Courier',
  },
  hint: {
    fontSize: 12,
    color: '#999',
    marginTop: 8,
    textAlign: 'center',
  },
});
