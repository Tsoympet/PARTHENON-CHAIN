/**
 * Send Screen
 */

import React, {useState} from 'react';
import {View, StyleSheet, ScrollView, SafeAreaView} from 'react-native';
import {Input, Button} from '@components';

export const SendScreen: React.FC = () => {
  const [recipient, setRecipient] = useState('');
  const [amount, setAmount] = useState('');
  const [memo, setMemo] = useState('');

  const handleSend = () => {
    // TODO: Implement send transaction logic
    console.log('Send transaction:', {recipient, amount, memo});
  };

  return (
    <SafeAreaView style={styles.container}>
      <ScrollView contentContainerStyle={styles.content}>
        <Input
          label="Recipient Address"
          value={recipient}
          onChangeText={setRecipient}
          placeholder="drm1234..."
          autoCapitalize="none"
        />

        <Input
          label="Amount"
          value={amount}
          onChangeText={setAmount}
          placeholder="0.00"
          keyboardType="decimal-pad"
        />

        <Input
          label="Memo (Optional)"
          value={memo}
          onChangeText={setMemo}
          placeholder="Add a note..."
          multiline
        />

        <Button
          title="Send"
          onPress={handleSend}
          disabled={!recipient || !amount}
          style={styles.sendButton}
        />
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
  sendButton: {
    marginTop: 24,
  },
});
