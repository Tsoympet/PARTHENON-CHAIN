/**
 * Mobile Mining Redux Slice
 * 
 * State management for mobile mining functionality.
 * This manages the specialized mobile mining service state.
 */

import {createSlice, PayloadAction} from '@reduxjs/toolkit';
import {MobileMiningConfig, MobileMiningStats} from '../../services/mining/MobileMiningService';

interface MiningState {
  config: MobileMiningConfig;
  stats: MobileMiningStats;
  isEnabled: boolean;
  error: string | null;
}

const initialState: MiningState = {
  config: {
    enabled: false,
    maxBatteryDrain: 5,
    enableOnBattery: false,
    enableOnCharging: true,
    minBatteryLevel: 30,
    maxTemperature: 40,
    hashBatchSize: 100,
    sleepBetweenBatches: 100,
    lowPowerMode: true,
    backgroundHashBatchSize: 10,
  },
  stats: {
    isActive: false,
    hashRate: 0,
    sharesFound: 0,
    sharesAccepted: 0,
    sharesRejected: 0,
    uptime: 0,
    temperature: 0,
    batteryLevel: 100,
    isCharging: false,
  },
  isEnabled: false,
  error: null,
};

const miningSlice = createSlice({
  name: 'mining',
  initialState,
  reducers: {
    setMiningEnabled: (state, action: PayloadAction<boolean>) => {
      state.isEnabled = action.payload;
      state.config.enabled = action.payload;
    },
    updateMiningConfig: (state, action: PayloadAction<Partial<MobileMiningConfig>>) => {
      state.config = {...state.config, ...action.payload};
    },
    updateMiningStats: (state, action: PayloadAction<MobileMiningStats>) => {
      state.stats = action.payload;
    },
    setMiningError: (state, action: PayloadAction<string | null>) => {
      state.error = action.payload;
    },
    resetMiningStats: (state) => {
      state.stats = initialState.stats;
    },
  },
});

export const {
  setMiningEnabled,
  updateMiningConfig,
  updateMiningStats,
  setMiningError,
  resetMiningStats,
} = miningSlice.actions;

export default miningSlice.reducer;
