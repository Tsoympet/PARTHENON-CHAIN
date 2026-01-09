/**
 * Home Screen
 */

import React, {useMemo} from 'react';
import {ScrollView, SafeAreaView, StyleSheet, Text, View} from 'react-native';
import {useNavigation} from '@react-navigation/native';
import {useSelector} from 'react-redux';
import {RootState} from '@store';
import {BalanceCard, Button} from '@components';
import LottieView from 'lottie-react-native';

export const HomeScreen: React.FC = () => {
  const navigation = useNavigation();
  const {currentAddress, balances} = useSelector((state: RootState) => state.wallet);
  const drmBalance = useMemo(
    () => balances.find(balance => balance.assetId === 'drm'),
    [balances]
  );

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Text style={styles.title}>Drachma Wallet</Text>

        <View style={styles.animationContainer}>
          <LottieView
            source={require('../../../assets/animations/mining-active.json')}
            autoPlay
            loop
            style={styles.animation}
          />
        </View>
        
        <BalanceCard
          balance={(drmBalance?.balance ?? 0).toFixed(4)}
          currency="DRM"
        />

        <View style={styles.actions}>
          <Button 
            title="Send" 
            onPress={() => navigation.navigate('Send' as never)} 
            style={styles.actionButton}
          />
          <Button 
            title="Receive" 
            onPress={() => navigation.navigate('Receive' as never)} 
            style={styles.actionButton}
            variant="outline"
          />
        </View>

        {currentAddress && (
          <View style={styles.addressSection}>
            <Text style={styles.sectionTitle}>Your Address</Text>
            <Text style={styles.address}>{currentAddress}</Text>
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
  animationContainer: {
    alignItems: 'center',
    marginBottom: 12,
  },
  animation: {
    width: 120,
    height: 120,
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
