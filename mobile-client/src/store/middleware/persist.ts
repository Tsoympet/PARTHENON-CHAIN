/**
 * Redux Persist configuration
 */

import AsyncStorage from '@react-native-async-storage/async-storage';
import {PersistConfig} from 'redux-persist';
import {RootState} from '../index';

export const persistConfig: PersistConfig<RootState> = {
  key: 'drachma-wallet',
  storage: AsyncStorage,
  whitelist: ['network', 'wallet', 'mining'],
};
