# DRACHMA Asset Directory

This directory contains all visual assets and branding materials for the DRACHMA blockchain project.

## Directory Structure

### `/core-icons/`
Application branding icons used by the desktop wallet:
- `app-icon.svg` - Main application icon (128x128)
- `tray-icon.svg` - System tray icon (32x32)
- `splash-icon.svg` - Splash screen icon (256x256)

### `/icons/`
Asset type icons representing the three core assets:
- `asset-tln.svg` - TLN (Talon) coin icon
- `asset-drm.svg` - DRM (Drachma) coin icon
- `asset-obl.svg` - OBL (Obol) coin icon

### `/nft-icons/`
NFT category icons for the sidechain marketplace:
- `nft-default.svg` - Default fallback for uncategorized NFTs
- `nft-hero.svg` - Hero category
- `nft-monument.svg` - Monument category
- `nft-mythology.svg` - Mythology category
- `nft-symbol.svg` - Symbol category

### `/ui-icons/`
User interface control icons (with light and dark variants):
- Network status: `network.svg`, `error.svg`, `warning.svg`, `shield.svg`
- Wallet: `wallet.svg`, `address-book.svg`, `key.svg`, `qr.svg`
- Transactions: `transactions.svg`, `tx-pending.svg`, `tx-out.svg`
- Staking: `stake-inactive.svg`, `validator.svg`, `rewards.svg`
- Mining: `hash.svg`, `difficulty.svg`
- System: `disk.svg`

Dark variants are in `/ui-icons/dark/` subdirectory.

## Asset Guidelines

### Icon Format
- **Format:** SVG (Scalable Vector Graphics)
- **Size:** Icons should be designed at their intended display size
- **Colors:** Use semantic colors that align with the brand
  - Primary: Green (#2E7D32, #388E3C)
  - Dark: (#1B5E20, #0D3415)
  - Light: (#E8F5E9, #C8E6C9)
  - Accent: Based on asset type or context

### Adding New Icons
1. Create SVG file with descriptive name
2. Place in appropriate subdirectory
3. Update this README with description
4. Test icon rendering in wallet UI
5. Commit with clear description

### Replacing Icons
To replace an icon while maintaining compatibility:
1. Keep the same filename
2. Maintain the same viewBox dimensions
3. Test in both light and dark themes
4. Ensure accessibility (sufficient contrast)

## Usage in Code

Icons are loaded at runtime by the Qt wallet application. The application references these paths directly:

```cpp
// Example from layer3-app/qt/main.cpp
QIcon appIcon(":/assets/core-icons/app-icon.svg");
QPixmap assetIcon(":/assets/icons/asset-drm.svg");
```

Do not rename or move files without updating the corresponding UI code.

## Asset Metadata

The `asset_metadata.json` file contains metadata about asset properties and display settings. This is used by the wallet to correctly render asset information.

## Notes

- Icons should work well on both light and dark backgrounds
- SVG format allows scaling without quality loss
- Keep file sizes reasonable (<50KB per icon)
- Icons should be recognizable at small sizes (16x16, 24x24)
- All icons are loaded from the filesystem; no embedded binary resources
- The delta symbol (Δ) represents the DRACHMA brand

## License

All icons in this directory are part of the DRACHMA project and follow the project's MIT license. See the root LICENSE file for details.
