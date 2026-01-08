/**
 * Receive Screen
 */

import React from 'react';
import {View, Text, StyleSheet, ScrollView, SafeAreaView} from 'react-native';
import {useSelector} from 'react-redux';
import {RootState} from '@store';
import {AddressDisplay, Button} from '@components';

export const ReceiveScreen: React.FC = () => {
  const {address} = useSelector((state: RootState) => state.wallet);

  const handleCopyAddress = () => {
    // TODO: Copy address to clipboard
    console.log('Copy address:', address);
  };

  const handleShare = () => {
    // TODO: Share address
    console.log('Share address:', address);
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Receive Drachma</Text>
        <Text style={styles.subtitle}>
          Share your address to receive payments
        </Text>

        {address && (
          <>
            <View style={styles.qrContainer}>
              {/* TODO: Add QR code component */}
              <View style={styles.qrPlaceholder}>
                <Text style={styles.qrText}>QR Code</Text>
              </View>
            </View>

            <AddressDisplay address={address} onCopy={handleCopyAddress} />

            <Button
              title="Share Address"
              onPress={handleShare}
              variant="outline"
              style={styles.shareButton}
            />
          </>
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
    fontSize: 24,
    fontWeight: 'bold',
    color: '#333',
    marginBottom: 8,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 14,
    color: '#666',
    marginBottom: 24,
    textAlign: 'center',
  },
  qrContainer: {
    alignItems: 'center',
    marginVertical: 24,
  },
  qrPlaceholder: {
    width: 200,
    height: 200,
    backgroundColor: '#FFF',
    borderRadius: 12,
    justifyContent: 'center',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#DDD',
  },
  qrText: {
    color: '#999',
    fontSize: 16,
  },
  shareButton: {
    marginTop: 16,
  },
});
