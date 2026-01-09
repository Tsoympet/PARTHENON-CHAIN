# Fonts

This directory contains custom fonts for the Drachma Mobile Wallet.

## Included Fonts

Currently, this directory is a placeholder. Custom fonts will be added as needed.

## Recommended Fonts

- **Primary**: System default (San Francisco on iOS, Roboto on Android)
- **Monospace**: For addresses and transaction IDs
- **Display**: For large headings (optional)

## Usage

To add custom fonts:

1. Place font files (.ttf, .otf) in this directory
2. Update `react-native.config.js` to include custom fonts
3. Run `npx react-native-asset` to link fonts
4. Reference fonts in styles by family name

## Guidelines

- Ensure fonts have proper licensing for commercial use
- Include all required font weights (regular, medium, bold)
- Test font rendering on both iOS and Android
- Keep total font size under 1MB for app performance
