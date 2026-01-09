/**
 * Wallet Screen
 */

import React, {useEffect, useMemo, useState} from 'react';
import {Alert, SafeAreaView, ScrollView, StyleSheet, Text, View} from 'react-native';
import {useDispatch, useSelector} from 'react-redux';
import {Button, Card} from '@components';
import {RootState} from '@store';
import {setCurrentAddress, setInitialized} from '@store/slices';
import {formatCrypto, truncateAddress} from '@utils/format';
import {WalletService} from '@services/wallet/WalletService';

const walletService = new WalletService();

export const WalletScreen: React.FC = () => {
  const dispatch = useDispatch();
  const {currentAddress, balances, isInitialized} = useSelector(
    (state: RootState) => state.wallet
  );
  const [isBusy, setIsBusy] = useState(false);

  useEffect(() => {
    const loadWallet = async () => {
      const wallet = await walletService.getWallet();
      if (wallet?.accounts?.length) {
        dispatch(setCurrentAddress(wallet.accounts[wallet.currentAccountIndex].address));
        dispatch(setInitialized(true));
      }
    };

    loadWallet().catch(err => {
      console.error('Failed to load wallet:', err);
    });
  }, [dispatch]);

  const primaryBalance = useMemo(
    () => balances.find(balance => balance.assetId === 'drm'),
    [balances]
  );

  const handleCreateWallet = async () => {
    setIsBusy(true);
    try {
      const mnemonic = await walletService.generateWallet();
      const account = await walletService.getCurrentAccount();
      if (account) {
        dispatch(setCurrentAddress(account.address));
        dispatch(setInitialized(true));
      }
      Alert.alert('Recovery Phrase', mnemonic, {cancelable: true});
    } catch (error) {
      Alert.alert('Wallet Error', 'Unable to create wallet.');
    } finally {
      setIsBusy(false);
    }
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Wallet</Text>

        {!isInitialized && (
          <Card style={styles.card}>
            <Text style={styles.cardTitle}>Create your wallet</Text>
            <Text style={styles.cardBody}>
              Generate a secure recovery phrase and start managing DRM, OBL, and TLN.
            </Text>
            <Button title="Create Wallet" onPress={handleCreateWallet} loading={isBusy} />
          </Card>
        )}

        {isInitialized && currentAddress && (
          <Card style={styles.card}>
            <Text style={styles.cardTitle}>Current Address</Text>
            <Text style={styles.address}>{truncateAddress(currentAddress, 12, 10)}</Text>
            <Text style={styles.addressFull}>{currentAddress}</Text>
          </Card>
        )}

        <Card style={styles.card}>
          <Text style={styles.cardTitle}>Balances</Text>
          {balances.map(balance => (
            <View key={balance.assetId} style={styles.balanceRow}>
              <Text style={styles.balanceLabel}>{balance.symbol}</Text>
              <Text style={styles.balanceValue}>
                {formatCrypto(balance.balance, 4, balance.symbol)}
              </Text>
            </View>
          ))}
          <Text style={styles.totalHint}>
            Primary balance: {formatCrypto(primaryBalance?.balance ?? 0, 4, 'DRM')}
          </Text>
        </Card>
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
  card: {
    marginBottom: 16,
  },
  cardTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 8,
  },
  cardBody: {
    fontSize: 14,
    color: '#666',
    marginBottom: 12,
  },
  address: {
    fontSize: 16,
    fontWeight: '600',
    color: '#007AFF',
  },
  addressFull: {
    fontSize: 12,
    color: '#666',
    marginTop: 6,
  },
  balanceRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 8,
  },
  balanceLabel: {
    fontSize: 14,
    color: '#555',
  },
  balanceValue: {
    fontSize: 14,
    fontWeight: '600',
    color: '#222',
  },
  totalHint: {
    fontSize: 12,
    color: '#888',
    marginTop: 8,
  },
});
