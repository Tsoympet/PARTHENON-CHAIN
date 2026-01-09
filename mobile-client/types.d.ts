declare module '*.svg' {
  import React from 'react';
  import {SvgProps} from 'react-native-svg';
  const content: React.FC<SvgProps>;
  export default content;
}

declare module '*.png' {
  const content: any;
  export default content;
}

declare module '*.jpg' {
  const content: any;
  export default content;
}

declare module '*.jpeg' {
  const content: any;
  export default content;
}

declare module '@env' {
  export const DEFAULT_NETWORK: string;
  export const TESTNET_RPC_URL: string;
  export const MAINNET_RPC_URL: string;
  export const APP_NAME: string;
  export const APP_VERSION: string;
  export const MOBILE_MINING_ENABLED: string;
  export const POOL_URL: string;
  export const WORKER_NAME: string;
}
