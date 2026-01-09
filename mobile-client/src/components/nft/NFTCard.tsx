/**
 * NFT Card Component
 */

import React from 'react';
import {View, Text, Image, StyleSheet, TouchableOpacity} from 'react-native';
import {Card} from '../common';

export interface NFTMetadata {
  id: string;
  name: string;
  description?: string;
  image?: string;
  collection?: string;
  tokenId: string;
}

interface NFTCardProps {
  nft: NFTMetadata;
  onPress?: () => void;
}

export const NFTCard: React.FC<NFTCardProps> = ({nft, onPress}) => {
  return (
    <TouchableOpacity onPress={onPress} activeOpacity={0.8}>
      <Card style={styles.container}>
        {nft.image ? (
          <Image source={{uri: nft.image}} style={styles.image} />
        ) : (
          <View style={[styles.image, styles.fallbackImage]}>
            <Text style={styles.fallbackText}>No Image</Text>
          </View>
        )}
        <View style={styles.info}>
          <Text style={styles.name} numberOfLines={1}>
            {nft.name}
          </Text>
          {nft.collection && (
            <Text style={styles.collection} numberOfLines={1}>
              {nft.collection}
            </Text>
          )}
          <Text style={styles.tokenId}>#{nft.tokenId}</Text>
        </View>
      </Card>
    </TouchableOpacity>
  );
};

const styles = StyleSheet.create({
  container: {
    padding: 0,
    overflow: 'hidden',
  },
  image: {
    width: '100%',
    height: 200,
    backgroundColor: '#F0F0F0',
  },
  fallbackImage: {
    justifyContent: 'center',
    alignItems: 'center',
  },
  fallbackText: {
    color: '#999',
    fontSize: 14,
  },
  info: {
    padding: 12,
  },
  name: {
    fontSize: 16,
    fontWeight: '600',
    color: '#333',
    marginBottom: 4,
  },
  collection: {
    fontSize: 12,
    color: '#666',
    marginBottom: 4,
  },
  tokenId: {
    fontSize: 12,
    color: '#999',
    fontFamily: 'Courier',
  },
});
