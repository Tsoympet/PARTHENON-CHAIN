/**
 * Send Screen
 */

import React, {useMemo, useState} from 'react';
import {StyleSheet, ScrollView, SafeAreaView, Text} from 'react-native';
import {useDispatch, useSelector} from 'react-redux';
import {Input, Button} from '@components';
import {RootState} from '@store';
import {addTransaction, updateBalance} from '@store/slices';
import {isValidAddress, isValidAmount, sanitizeInput} from '@utils/validation';

export const SendScreen: React.FC = () => {
  const dispatch = useDispatch();
  const {currentAddress, balances} = useSelector((state: RootState) => state.wallet);
  const [recipient, setRecipient] = useState('');
  const [amount, setAmount] = useState('');
  const [memo, setMemo] = useState('');
  const [error, setError] = useState<string | null>(null);

  const drmBalance = useMemo(
    () => balances.find(balance => balance.assetId === 'drm'),
    [balances]
  );

  const handleSend = () => {
    const sanitizedRecipient = sanitizeInput(recipient.trim());
    const sanitizedMemo = sanitizeInput(memo.trim());

    if (!currentAddress) {
      setError('Create or restore a wallet before sending.');
      return;
    }

    if (!isValidAddress(sanitizedRecipient)) {
      setError('Recipient address is invalid.');
      return;
    }

    if (!isValidAmount(amount)) {
      setError('Amount must be greater than zero.');
      return;
    }

    const numericAmount = parseFloat(amount);
    const available = drmBalance?.balance ?? 0;

    if (numericAmount > available) {
      setError('Insufficient DRM balance.');
      return;
    }

    const txId = `tx-${Date.now()}`;

    dispatch(
      addTransaction({
        id: txId,
        type: 'send',
        from: currentAddress,
        to: sanitizedRecipient,
        amount: numericAmount.toFixed(8),
        assetId: 'drm',
        timestamp: Date.now(),
        status: 'pending',
        memo: sanitizedMemo || undefined,
      })
    );

    dispatch(
      updateBalance({
        assetId: 'drm',
        symbol: 'DRM',
        name: 'Drachma',
        balance: Math.max(0, available - numericAmount),
        decimals: 8,
      })
    );

    setRecipient('');
    setAmount('');
    setMemo('');
    setError(null);
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
          placeholder="0.00 DRM"
          keyboardType="decimal-pad"
        />

        <Input
          label="Memo (Optional)"
          value={memo}
          onChangeText={setMemo}
          placeholder="Add a note..."
          multiline
        />

        {error && <Text style={styles.errorText}>{error}</Text>}

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
  errorText: {
    color: '#D32F2F',
    marginTop: 8,
    fontSize: 13,
  },
});
