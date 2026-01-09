# Mobile Client Structure Documentation

This document describes the complete directory structure of the Drachma Mobile Wallet.

## Overview

The mobile client is built with React Native and TypeScript, following a modular architecture for maintainability and scalability.

## Directory Structure

The full, generated file tree is stored in `STRUCTURE.txt` so the listing reflects the
real files rather than a hand-maintained Markdown block.

## Key Features

### Components

#### Common Components
- **Button**: Customizable button with loading states and variants
- **Input**: Form input with label, error handling, and validation
- **Card**: Container component with consistent styling

#### Wallet Components
- **BalanceCard**: Displays wallet balance with USD conversion
- **AddressDisplay**: Shows wallet address with copy functionality
- **TransactionItem**: Individual transaction list item

#### NFT Components
- **NFTCard**: Single NFT display card
- **NFTGallery**: Grid view of NFT collection

### Screens

1. **Home**: Balance overview and quick actions
2. **Wallet**: Account creation, address, and balances
3. **Send**: Transaction creation form
4. **Receive**: Address display with QR code
5. **Transactions**: Transaction history list
6. **Settings**: App configuration and security settings
7. **NFT**: NFT gallery view

### Services

- **WalletService**: BIP39 mnemonic generation, HD wallet derivation, transaction signing
- **RPCClient**: Blockchain communication (balance queries, transaction submission)
- **CryptoService**: Cryptographic primitives (Schnorr/ECDSA signatures, hashing)
- **MobileMiningService**: Mobile-optimized mining (battery-aware, background mode)
- **SecureStorage**: Encrypted storage using Expo SecureStore

### State Management

Redux Toolkit with:
- **walletSlice**: Wallet state (addresses, balances, transactions)
- **networkSlice**: Network state (connection, sync status, RPC)
- **miningSlice**: Mining state (hashrate, stats, configuration)

Custom middleware:
- **logger**: Development logging
- **persist**: State persistence

### Utilities

- **Format utilities**: Currency formatting, address truncation, date formatting
- **Validation utilities**: Address validation, amount validation, mnemonic validation

### TypeScript Types

Comprehensive type definitions for:
- Wallet accounts and state
- Network information
- Transactions
- Mining configuration
- NFT metadata

## Path Aliases

The project uses TypeScript path aliases for clean imports:

```typescript
import { Button } from '@components';
import { HomeScreen } from '@screens';
import { WalletService } from '@services';
import { RootState } from '@store';
import { formatCrypto } from '@utils';
import { NETWORKS } from '@constants';
import { Transaction } from '@types';
```

## Testing

Tests are organized in `__tests__/` directory:
- Component tests using React Testing Library
- Service tests for business logic
- Utility function tests
- Integration tests

Run tests with:
```bash
npm test
```

## Development Workflow

1. **Install dependencies**: `npm install`
2. **Type checking**: `npm run type-check`
3. **Linting**: `npm run lint`
4. **Run on Android**: `npm run android`
5. **Run on iOS**: `npm run ios`
6. **Run tests**: `npm test`

## Security Considerations

- Private keys stored in secure storage (Expo SecureStore)
- Sensitive data encrypted using platform keychain/keystore
- BIP39 mnemonic generation with proper entropy
- Schnorr signatures for transactions
- Input validation and sanitization

## Next Steps

1. Implement QR code scanning for address input
2. Add secure app lock and quick unlock flow
3. Implement real-time balance updates
4. Add transaction notifications
5. Implement NFT metadata fetching
6. Add deep linking support
7. Implement wallet backup/restore flows
8. Add internationalization (i18n)

## Contributing

When adding new features:
1. Create components in appropriate directories
2. Add TypeScript types in `src/types/`
3. Write tests in `__tests__/`
4. Update this documentation

## License

MIT - See LICENSE file for details
