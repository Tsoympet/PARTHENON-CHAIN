/**
 * Receive Screen
 */

import React from 'react';
import {Share, StyleSheet, ScrollView, SafeAreaView, Text, View} from 'react-native';
import {useSelector} from 'react-redux';
import * as Clipboard from 'expo-clipboard';
import QRCode from 'react-native-qrcode-svg';
import {RootState} from '@store';
import {AddressDisplay, Button} from '@components';

export const ReceiveScreen: React.FC = () => {
  const {currentAddress} = useSelector((state: RootState) => state.wallet);

  const handleCopyAddress = async () => {
    if (!currentAddress) return;
    await Clipboard.setStringAsync(currentAddress);
  };

  const handleShare = async () => {
    if (!currentAddress) return;
    await Share.share({
      message: `My Drachma address: ${currentAddress}`,
    });
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Receive Drachma</Text>
        <Text style={styles.subtitle}>
          Share your address to receive payments
        </Text>

        {currentAddress && (
          <>
            <View style={styles.qrContainer}>
              <QRCode value={currentAddress} size={200} testID="qr-code" />
            </View>

            <AddressDisplay address={currentAddress} onCopy={handleCopyAddress} />

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
  shareButton: {
    marginTop: 16,
  },
});
