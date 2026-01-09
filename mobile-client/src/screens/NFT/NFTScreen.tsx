/**
 * NFT Screen
 */

import React from 'react';
import {SafeAreaView, StyleSheet, Text, View} from 'react-native';
import {NFTGallery} from '@components';
import {NFTMetadata} from '@components';
import {LOCAL_NFTS} from '@constants';

export const NFTScreen: React.FC = () => {
  const handleNFTPress = (nft: NFTMetadata) => {
    console.log('NFT pressed:', nft.id);
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>NFT Gallery</Text>
      </View>
      <NFTGallery nfts={LOCAL_NFTS} onNFTPress={handleNFTPress} />
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
});
