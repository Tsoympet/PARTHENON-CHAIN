# PARTHENON CHAIN Mobile Wallet

A mobile wallet for PARTHENON CHAIN (Drachma) blockchain, supporting iOS and Android platforms.

## Features

- **Multi-Asset Support**: Manage TLN (Talanton), DRM (Drachma), and OBL (Obolos) assets
- **Secure Key Management**: Industry-standard HD wallet with BIP39 mnemonic support
- **Transaction Management**: Send, receive, and track transactions
- **QR Code Support**: Easy address sharing and scanning
- **Biometric Authentication**: Fingerprint and Face ID support
- **Offline Mode**: View balances and generate addresses offline
- **NFT Gallery**: View and manage NFTs from Layer 2
- **Staking Interface**: Monitor and manage staking rewards
- **Mobile Mining**: Specialized lightweight mining for mobile devices (battery-aware, temperature-monitored)

## Technology Stack

- **Framework**: React Native 0.72+
- **Language**: TypeScript
- **State Management**: Redux Toolkit
- **Navigation**: React Navigation
- **Storage**: React Native MMKV (encrypted)
- **Crypto**: react-native-crypto, @noble/secp256k1
- **Networking**: Axios with custom RPC client

## Prerequisites

- Node.js 18+ and npm/yarn
- iOS: Xcode 14+, CocoaPods
- Android: Android Studio, JDK 17+

## Installation

### Clone and Setup

```bash
cd mobile-client
npm install
# or
yarn install
```

### iOS Setup

```bash
cd ios
pod install
cd ..
npx react-native run-ios
```

### Android Setup

```bash
npx react-native run-android
```

## Project Structure

```
mobile-client/
├── src/
│   ├── components/        # Reusable UI components
│   │   ├── common/        # Buttons, inputs, cards
│   │   ├── wallet/        # Wallet-specific components
│   │   └── nft/           # NFT gallery components
│   ├── screens/           # App screens
│   │   ├── Home/
│   │   ├── Send/
│   │   ├── Receive/
│   │   ├── Transactions/
│   │   ├── Settings/
│   │   └── NFT/
│   ├── navigation/        # Navigation configuration
│   ├── services/          # Business logic
│   │   ├── wallet/        # Wallet management
│   │   ├── rpc/           # RPC client
│   │   ├── crypto/        # Cryptographic operations
│   │   ├── mining/        # Mobile mining (specialized, not PC miners)
│   │   └── storage/       # Secure storage
│   ├── store/             # Redux store
│   │   ├── slices/        # Feature slices
│   │   └── middleware/    # Custom middleware
│   ├── utils/             # Utility functions
│   ├── constants/         # App constants
│   └── types/             # TypeScript types
├── android/               # Android native code
├── ios/                   # iOS native code
├── assets/                # Images, fonts, etc.
└── __tests__/             # Test files
```

## Configuration

Create a `.env` file in the mobile-client directory:

```env
# Network Configuration
DEFAULT_NETWORK=testnet
TESTNET_RPC_URL=https://tn1.drachma.org:18332
MAINNET_RPC_URL=https://node1.drachma.org:8332

# App Configuration
APP_NAME=Drachma Wallet
APP_VERSION=0.1.0
```

## Security Features

### Key Storage
- Private keys never leave the device
- Encrypted storage using device keychain/keystore
- Optional passphrase protection
- Biometric authentication for transactions

### Network Security
- TLS/SSL for all network communication
- Certificate pinning for RPC endpoints
- Request signing and verification
- Rate limiting and anti-spam measures

### Privacy
- No analytics or tracking
- Local transaction history
- Optional Tor support (future)

## Development

### Running Tests

```bash
npm test
# or
yarn test
```

### Linting

```bash
npm run lint
# or
yarn lint
```

### Building for Production

#### iOS

```bash
cd ios
xcodebuild -workspace DrachmaWallet.xcworkspace \
  -scheme DrachmaWallet \
  -configuration Release \
  -archivePath build/DrachmaWallet.xcarchive \
  archive
```

#### Android

```bash
cd android
./gradlew assembleRelease
```

## Mobile Mining

The mobile wallet includes a **specialized mobile mining implementation** that is fundamentally different from PC miners.

**Key Features:**
- Battery-aware mining with automatic throttling
- Temperature monitoring and auto-stop
- Background/foreground adaptive behavior
- Optimized for ARM processors (not x86 AVX2/CUDA/OpenCL)
- Small hash batches with sleep intervals

**Important:** This is NOT the same as PC miners. See [MOBILE_MINING.md](MOBILE_MINING.md) for complete documentation.

### Quick Start

```typescript
import {MobileMiningService} from './services/mining/MobileMiningService';

const miningService = new MobileMiningService(rpcClient, {
  enableOnCharging: true,
  enableOnBattery: false,
  minBatteryLevel: 30,
});

await miningService.startMining();
```

For detailed configuration and usage, see [MOBILE_MINING.md](MOBILE_MINING.md).

## Features Roadmap

### Version 0.1.0 (Current)
- [x] Basic wallet functionality
- [x] Send/receive transactions
- [x] Multi-asset support (TLN/DRM/OBL)
- [x] QR code scanning
- [ ] Biometric authentication
- [ ] Transaction history

### Version 0.2.0
- [ ] Mobile mining (specialized for phones)
- [ ] NFT gallery and marketplace
- [ ] Staking interface
- [ ] Advanced fee estimation
- [ ] Multi-wallet support
- [ ] Contact book

### Version 0.3.0
- [ ] Hardware wallet support
- [ ] dApp browser integration
- [ ] Advanced privacy features
- [ ] Multi-signature support

## Known Limitations

- **Testnet First**: Currently optimized for testnet; mainnet support requires additional hardening
- **RPC Dependency**: Requires connection to a full node; no SPV support yet
- **Platform Support**: iOS 13+ and Android 8+
- **Language**: English only (i18n planned)

## Contributing

Please follow the [Contributing Guide](../doc/CONTRIBUTING.md) from the main repository.

## Security

For security issues, please email: security@drachma.org

Do not open public issues for security vulnerabilities.

## License

MIT License - see [LICENSE](../LICENSE) for details

## Support

- Documentation: [Main Repository](https://github.com/Tsoympet/PARTHENON-CHAIN)
- Discord: https://discord.gg/drachma
- Matrix: https://matrix.to/#/#drachma:matrix.org

## Acknowledgments

Built with ❤️ for the PARTHENON CHAIN community.
