# Mobile App Assets

This directory contains static assets for the Drachma Mobile Wallet.

## Structure

```
assets/
├── images/         # App images and icons
├── fonts/          # Custom fonts
├── animations/     # Lottie animations
└── icons/          # Icon sets
```

## Usage

Assets can be imported in React Native components:

```typescript
import logo from './assets/images/logo.png';
```

## Image Guidelines

- Use PNG for images with transparency
- Use JPEG for photos
- Use SVG for scalable icons
- Provide @2x and @3x versions for different screen densities
