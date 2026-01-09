# Mobile App Assets

This directory contains static assets for the Drachma Mobile Wallet.

## Structure

```
assets/
├── icons/                    # Icon sets
│   ├── core/                # Core app icons (app-icon, splash, tray)
│   ├── assets/              # Asset type icons (TLN, DRM, OBL)
│   ├── nft/                 # NFT-related icons
│   └── ui/                  # UI icons for wallet features
│       ├── light/           # Light theme variants
│       └── dark/            # Dark theme variants
├── images/                  # App images and graphics (placeholder)
├── fonts/                   # Custom fonts (placeholder)
└── animations/              # Lottie animations (placeholder)
```

## Icon Categories

### Core Icons (`icons/core/`)
- **app-icon.svg**: Main application icon
- **splash-icon.svg**: Splash screen icon
- **tray-icon.svg**: System tray/notification icon

### Asset Icons (`icons/assets/`)
- **asset-tln.svg**: Talanton (TLN) token icon
- **asset-drm.svg**: Drachma (DRM) token icon
- **asset-obl.svg**: Obolos (OBL) token icon

### NFT Icons (`icons/nft/`)
- **nft-default.svg**: Default NFT placeholder
- **nft-hero.svg**: Hero-themed NFT icon
- **nft-monument.svg**: Monument-themed NFT icon
- **nft-mythology.svg**: Mythology-themed NFT icon
- **nft-symbol.svg**: Symbol-themed NFT icon

### UI Icons (`icons/ui/`)
Comprehensive set of UI icons for wallet features including:

**Wallet Operations:**
- wallet.svg, send.svg, receive.svg, transactions.svg
- balance.svg, qr.svg, address-book.svg

**Transaction States:**
- tx-in.svg, tx-out.svg, tx-pending.svg
- tx-confirmed.svg, tx-failed.svg

**Network & Sync:**
- network.svg, network-connected.svg, network-disconnected.svg
- sync.svg, synced.svg, peers.svg
- status-connected.svg, status-disconnected.svg

**Security:**
- lock.svg, unlock.svg, key.svg
- shield.svg, security.svg
- backup.svg, restore.svg

**Staking:**
- staking.svg, stake-active.svg, stake-inactive.svg
- rewards.svg, validator.svg

**Mining:**
- mining.svg, cpu.svg, hash.svg
- difficulty.svg, block.svg

**General UI:**
- settings.svg, info.svg, warning.svg, error.svg
- history.svg, log.svg, logo.svg

**Theme Support:**
Each icon has light and dark theme variants in `ui/light/` and `ui/dark/` subdirectories.

## Usage

### Basic Import (React Native)

```typescript
// For SVG icons (using react-native-svg)
import Logo from './assets/icons/core/app-icon.svg';

// For PNG images
import WalletIcon from './assets/icons/ui/wallet.png';
```

### Using with react-native-svg

```typescript
import { SvgXml } from 'react-native-svg';
import SendIcon from './assets/icons/ui/send.svg';

<SvgXml xml={SendIcon} width={24} height={24} />
```

### Using with react-native-vector-icons

For themed icons, dynamically load based on theme:

```typescript
import { useColorScheme } from 'react-native';

const theme = useColorScheme();
const iconPath = theme === 'dark' 
  ? './assets/icons/ui/dark/wallet.svg'
  : './assets/icons/ui/light/wallet.svg';
```

## Image Guidelines

- **SVG Icons**: Use for all vector graphics and icons
  - Already included: All UI, core, asset, and NFT icons
  - Scalable to any size without quality loss
  - Small file size
  
- **PNG Images**: Use for raster images and photos
  - Provide @2x and @3x versions for different screen densities
  - Place in `images/` directory
  
- **Fonts**: Custom fonts go in `fonts/` directory
  - Include all required font weights
  - Update `react-native.config.js` to link fonts
  
- **Animations**: Lottie JSON files go in `animations/` directory
  - Use for onboarding, loading states, success/error states

## Adding New Assets

1. **Icons**: Add to appropriate subdirectory under `icons/`
2. **Images**: Add to `images/` with @2x and @3x variants
3. **Fonts**: Add to `fonts/` and configure in project
4. **Animations**: Add Lottie JSON to `animations/`

## Asset Optimization

Before adding new assets:
- **SVG**: Optimize with SVGO
- **PNG**: Compress with pngquant or similar
- **Animations**: Keep Lottie files small (< 50KB)

## Branding

All icons follow the PARTHENON CHAIN branding guidelines:
- **Theme**: Classical Greece, timeless design
- **Colors**: Marble, Bronze, Silver, Obsidian
- **Style**: Clean, minimalist, professional

See `/doc/BRANDING-GUIDE.md` for complete branding guidelines.

## License

All assets are part of the PARTHENON CHAIN project and licensed under MIT.
See the main LICENSE file for details.
