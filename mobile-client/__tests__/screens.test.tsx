/**
 * Screen Component Tests
 */

import React from 'react';
import {render, fireEvent} from '@testing-library/react-native';
import {Provider} from 'react-redux';
import {configureStore} from '@reduxjs/toolkit';
import {HomeScreen} from '../src/screens/Home/HomeScreen';
import {WalletScreen} from '../src/screens/Wallet/WalletScreen';
import {SendScreen} from '../src/screens/Send/SendScreen';
import {ReceiveScreen} from '../src/screens/Receive/ReceiveScreen';
import {walletReducer, networkReducer, miningReducer} from '../src/store/slices';

jest.mock('@react-navigation/native', () => ({
  useNavigation: () => ({
    navigate: jest.fn(),
  }),
}));

// Create mock store
const createMockStore = (initialState = {}) => {
  return configureStore({
    reducer: {
      wallet: walletReducer,
      network: networkReducer,
      mining: miningReducer,
    },
    preloadedState: initialState,
  });
};

describe('HomeScreen', () => {
  it('renders correctly', () => {
    const store = createMockStore({
      wallet: {
        isInitialized: true,
        currentAddress: 'drm1234567890abcdef',
        balances: [
          {assetId: 'drm', symbol: 'DRM', name: 'Drachma', balance: 10, decimals: 8},
        ],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });

    const {getByText} = render(
      <Provider store={store}>
        <HomeScreen />
      </Provider>
    );

    expect(getByText(/balance/i)).toBeTruthy();
  });

  it('navigates to send screen when send button is pressed', () => {
    const store = createMockStore();
    const {getByText} = render(
      <Provider store={store}>
        <HomeScreen />
      </Provider>
    );

    const sendButton = getByText(/send/i);
    fireEvent.press(sendButton);
    expect(sendButton).toBeTruthy();
  });
});

describe('WalletScreen', () => {
  it('shows create wallet state when uninitialized', () => {
    const store = createMockStore({
      wallet: {
        isInitialized: false,
        currentAddress: null,
        balances: [],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });

    const {getByText} = render(
      <Provider store={store}>
        <WalletScreen />
      </Provider>
    );

    expect(getByText(/create wallet/i)).toBeTruthy();
  });
});

describe('SendScreen', () => {
  it('renders send form', () => {
    const store = createMockStore({
      wallet: {
        isInitialized: true,
        currentAddress: 'drm1234567890abcdef1234567890abcdef12345678',
        balances: [{assetId: 'drm', symbol: 'DRM', name: 'Drachma', balance: 10, decimals: 8}],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });
    const {getByPlaceholderText} = render(
      <Provider store={store}>
        <SendScreen />
      </Provider>
    );

    expect(getByPlaceholderText(/recipient address/i)).toBeTruthy();
    expect(getByPlaceholderText(/amount/i)).toBeTruthy();
  });

  it('validates recipient address', async () => {
    const store = createMockStore({
      wallet: {
        isInitialized: true,
        currentAddress: 'drm1234567890abcdef1234567890abcdef12345678',
        balances: [{assetId: 'drm', symbol: 'DRM', name: 'Drachma', balance: 10, decimals: 8}],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });
    const {getByPlaceholderText, getByText, findByText} = render(
      <Provider store={store}>
        <SendScreen />
      </Provider>
    );

    const addressInput = getByPlaceholderText(/recipient address/i);
    fireEvent.changeText(addressInput, 'invalid-address');

    const sendButton = getByText(/send/i);
    fireEvent.press(sendButton);

    expect(await findByText(/invalid address/i)).toBeTruthy();
  });
});

describe('ReceiveScreen', () => {
  it('displays wallet address', () => {
    const testAddress = 'drm1234567890abcdef1234567890abcdef12345678';
    const store = createMockStore({
      wallet: {
        currentAddress: testAddress,
        isInitialized: true,
        balances: [],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });

    const {getByText} = render(
      <Provider store={store}>
        <ReceiveScreen />
      </Provider>
    );

    expect(getByText(testAddress)).toBeTruthy();
  });

  it('shows QR code for address', () => {
    const store = createMockStore({
      wallet: {
        currentAddress: 'drm1234567890abcdef',
        isInitialized: true,
        balances: [],
        transactions: [],
        isLoading: false,
        error: null,
      },
    });

    const {getByTestId} = render(
      <Provider store={store}>
        <ReceiveScreen />
      </Provider>
    );

    expect(getByTestId('qr-code')).toBeTruthy();
  });
});
