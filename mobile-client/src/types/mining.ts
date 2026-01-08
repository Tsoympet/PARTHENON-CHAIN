/**
 * Mining-related TypeScript types
 */

export interface MiningState {
  isActive: boolean;
  hashRate: number;
  totalMined: string;
  difficulty: number;
  currentBlockReward: string;
  isLoading: boolean;
  error: string | null;
}

export interface MiningConfig {
  enabled: boolean;
  threads: number;
  intensity: 'low' | 'medium' | 'high';
  poolUrl?: string;
}

export interface MiningStats {
  hashRate: number;
  sharesSubmitted: number;
  sharesAccepted: number;
  uptime: number;
  totalEarned: string;
}
