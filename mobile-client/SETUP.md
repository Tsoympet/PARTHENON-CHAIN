# Mobile Client Setup Guide

This guide will help you set up the Drachma Mobile Wallet development environment.

## Prerequisites

Before you begin, ensure you have the following installed:

### General Requirements
- **Node.js** 18.x or later
- **npm** 8.x or later (or **Yarn** 1.22.x or later)
- **Git**

### iOS Development (macOS only)
- **Xcode** 14.x or later
- **iOS Simulator** or physical iOS device

### Android Development
- **Android Studio** (latest version)
- **JDK** 17 or later
- **Android SDK** (API level 31+)
- **Android Emulator** or physical Android device

## Installation

### 1. Clone the Repository

```bash
git clone https://github.com/Tsoympet/PARTHENON-CHAIN.git
cd PARTHENON-CHAIN/mobile-client
```

### 2. Install Dependencies

```bash
npm install
# or
yarn install
```

### 3. Configure Environment

Copy the example environment file and configure it:

```bash
cp .env.example .env
```

Edit `.env` and set your preferred values:

```env
DEFAULT_NETWORK=testnet
TESTNET_RPC_URL=https://tn1.drachma.org:18332
MAINNET_RPC_URL=https://node1.drachma.org:8332
APP_NAME=Drachma Wallet
APP_VERSION=0.1.0
MOBILE_MINING_ENABLED=false
```

### 4. Platform-Specific Setup

Expo handles native configuration through prebuild when needed:

```bash
npm run prebuild
```

## Running the App

### Start Expo

In a terminal, start the Expo dev server:

```bash
npm start
# or
yarn start
```

Leave this running in the background.

### Run on iOS

In a new terminal:

```bash
npm run ios
# or
yarn ios
```

### Run on Android

In a new terminal:

```bash
npm run android
# or
yarn android
```

Make sure you have an Android emulator running or a device connected via USB with USB debugging enabled.

## Development Workflow

### Recommended IDE Setup

- **VS Code** with extensions:
  - React Native Tools
  - ESLint
  - Prettier
  - TypeScript and JavaScript Language Features

### Code Quality

#### Linting

```bash
npm run lint
# or
yarn lint
```

#### Type Checking

```bash
npm run type-check
# or
yarn type-check
```

#### Testing

```bash
npm test
# or
yarn test
```

### Hot Reloading

The app supports Fast Refresh (hot reloading). Changes to most files will be reflected immediately in the running app.

To manually reload:
- **iOS Simulator**: Press `Cmd + R`
- **Android Emulator**: Press `R` twice or `Cmd + M` (Mac) / `Ctrl + M` (Windows/Linux) to open dev menu

## Debugging

### React Native Debugger (Optional)

1. Install React Native Debugger:
   ```bash
   brew install --cask react-native-debugger  # macOS
   ```

2. Start the debugger
3. In the app, open the developer menu and select "Debug"

### Chrome DevTools

1. In the app, open the developer menu
2. Select "Open JS debugger"
3. Chrome will open the devtools panel

### Native Debugging

#### iOS
1. Run `npm run prebuild`
2. Open `ios/DrachmaWallet.xcworkspace` in Xcode
3. Run the app from Xcode

#### Android
1. Run `npm run prebuild`
2. Open `android/` in Android Studio
3. Run the app from Android Studio

## Troubleshooting

### Common Issues

#### Metro Bundler Issues

```bash
# Clear Metro cache
npm start -- --reset-cache
# or
yarn start --reset-cache
```

#### iOS Build Issues

```bash
# Clean build
cd ios
xcodebuild clean
rm -rf ~/Library/Developer/Xcode/DerivedData/*
pod deintegrate
pod install
cd ..
```

#### Android Build Issues

```bash
# Clean build
cd android
./gradlew clean
cd ..

# Clear Gradle cache
rm -rf ~/.gradle/caches/
```

#### Node Modules Issues

```bash
# Remove and reinstall dependencies
rm -rf node_modules
npm install
# or
yarn install
```

### Build Errors

If you encounter build errors:

1. Ensure all prerequisites are installed
2. Check that you're using the correct Node.js version (18+)
3. Clear all caches (Metro, Gradle, CocoaPods)
4. Remove `node_modules` and reinstall
5. Check GitHub issues for similar problems

## Project Structure

See [STRUCTURE.md](STRUCTURE.md) for detailed information about the project structure.

## Assets

All icons and assets are located in `assets/`. See [assets/README.md](assets/README.md) for details.

## Contributing

See the main [CONTRIBUTING.md](../doc/CONTRIBUTING.md) for contribution guidelines.

## License

MIT License - See [LICENSE](../LICENSE) for details.

## Support

- **Documentation**: [Main Repository](https://github.com/Tsoympet/PARTHENON-CHAIN)
- **Issues**: https://github.com/Tsoympet/PARTHENON-CHAIN/issues
- **Discord**: https://discord.gg/drachma

## Next Steps

After successful setup:

1. Read [README.md](README.md) for feature overview
2. Read [STRUCTURE.md](STRUCTURE.md) for code organization
3. Read [MOBILE_MINING.md](MOBILE_MINING.md) for mobile mining details
4. Start developing!

## Quick Commands Reference

```bash
# Install dependencies
npm install

# Start Metro bundler
npm start

# Run on iOS
npm run ios

# Run on Android
npm run android

# Run tests
npm test

# Lint code
npm run lint

# Type check
npm run type-check

# Clean project
npm run clean
```
