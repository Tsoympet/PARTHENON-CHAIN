/**
 * NFT Gallery Grid Component
 */

import React from 'react';
import {FlatList, StyleSheet, View} from 'react-native';
import {NFTCard, NFTMetadata} from './NFTCard';

interface NFTGalleryProps {
  nfts: NFTMetadata[];
  onNFTPress?: (nft: NFTMetadata) => void;
  numColumns?: number;
}

export const NFTGallery: React.FC<NFTGalleryProps> = ({
  nfts,
  onNFTPress,
  numColumns = 2,
}) => {
  return (
    <FlatList
      data={nfts}
      renderItem={({item}) => (
        <View style={styles.gridItem}>
          <NFTCard nft={item} onPress={() => onNFTPress?.(item)} />
        </View>
      )}
      keyExtractor={item => item.id}
      numColumns={numColumns}
      contentContainerStyle={styles.container}
    />
  );
};

const styles = StyleSheet.create({
  container: {
    padding: 8,
  },
  gridItem: {
    flex: 1,
    margin: 8,
    maxWidth: '50%',
  },
});
