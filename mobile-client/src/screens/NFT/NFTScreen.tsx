/**
 * NFT Screen
 */

import React from 'react';
import {View, Text, StyleSheet, SafeAreaView} from 'react-native';
import {NFTGallery, NFTMetadata} from '@components';

// Mock NFT data - replace with Redux state
const mockNFTs: NFTMetadata[] = [
  {
    id: '1',
    name: 'Drachma NFT #1',
    tokenId: '0001',
    collection: 'Genesis Collection',
  },
  {
    id: '2',
    name: 'Parthenon Art',
    tokenId: '0042',
    collection: 'Historical Series',
  },
];

export const NFTScreen: React.FC = () => {
  const handleNFTPress = (nft: NFTMetadata) => {
    // TODO: Navigate to NFT details
    console.log('NFT pressed:', nft.id);
  };

  return (
    <SafeAreaView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>NFT Gallery</Text>
      </View>
      <NFTGallery nfts={mockNFTs} onNFTPress={handleNFTPress} />
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
