# Animations

This directory contains Lottie animations for the Drachma Mobile Wallet.

## Purpose

Lottie animations provide smooth, lightweight animations exported from After Effects.

## Current Status

This directory is a placeholder. Animations will be added as the UI is developed.

## Planned Animations

- **loading.json** - Loading spinner animation
- **success.json** - Success checkmark animation
- **error.json** - Error state animation
- **transaction-sent.json** - Transaction confirmation animation
- **mining-active.json** - Mining in progress animation
- **wallet-created.json** - New wallet creation celebration

## Usage

```typescript
import LottieView from 'lottie-react-native';

<LottieView
  source={require('./animations/loading.json')}
  autoPlay
  loop
  style={styles.animation}
/>
```

## Guidelines

- Keep animation files under 100KB each
- Use solid colors when possible (smaller file size)
- Test animations on low-end devices
- Provide static fallback images for older devices
- Export from After Effects using Bodymovin plugin

## Resources

- [Lottie Files](https://lottiefiles.com/) - Free animations
- [LottieFiles React Native](https://github.com/lottie-react-native/lottie-react-native) - Library documentation
