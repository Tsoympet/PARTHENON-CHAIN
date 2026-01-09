# Fonts

This directory contains custom fonts for the Drachma Mobile Wallet.

## Included Fonts

The mobile client currently relies on system fonts for performance and consistency.
Custom font files can be added here when brand typography is finalized.

## Recommended Fonts

- **Primary**: System default (San Francisco on iOS, Roboto on Android)
- **Monospace**: For addresses and transaction IDs
- **Display**: For large headings (optional)

## Usage

To add custom fonts:

1. Place font files (.ttf, .otf) in this directory
2. Add the font paths to the Expo config (`app.json` or `app.config.js`)
3. Run `expo prebuild` if you need native projects
4. Reference fonts in styles by family name

## Guidelines

- Ensure fonts have proper licensing for commercial use
- Include all required font weights (regular, medium, bold)
- Test font rendering on both iOS and Android
- Keep total font size under 1MB for app performance
