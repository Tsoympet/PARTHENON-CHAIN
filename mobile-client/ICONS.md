# Mobile Client Icon Index

This file provides a quick reference for all available icons in the mobile client.

## How to Use Icons

### Import SVG Icons

```typescript
import WalletIcon from '../assets/icons/ui/wallet.svg';
import SendIcon from '../assets/icons/ui/send.svg';

// In your component
<WalletIcon width={24} height={24} fill="#000" />
```

### Theme-Aware Icons

```typescript
import { useColorScheme } from 'react-native';

const MyComponent = () => {
  const isDark = useColorScheme() === 'dark';
  
  const WalletIcon = isDark
    ? require('../assets/icons/ui/dark/wallet.svg').default
    : require('../assets/icons/ui/light/wallet.svg').default;
  
  return <WalletIcon width={24} height={24} />;
};
```

## Available Icons

### Core Icons (3)
Location: `assets/icons/core/`

- `app-icon.svg` - Main application icon
- `splash-icon.svg` - Splash screen icon  
- `tray-icon.svg` - System tray/notification icon

### Asset Icons (3)
Location: `assets/icons/assets/`

- `asset-tln.svg` - Talanton (TLN) token
- `asset-drm.svg` - Drachma (DRM) token
- `asset-obl.svg` - Obolos (OBL) token

### NFT Icons (5)
Location: `assets/icons/nft/`

- `nft-default.svg` - Default NFT placeholder
- `nft-hero.svg` - Hero-themed NFT
- `nft-monument.svg` - Monument-themed NFT
- `nft-mythology.svg` - Mythology-themed NFT
- `nft-symbol.svg` - Symbol-themed NFT

### UI Icons (51 icons × 3 variants = 153 total)
Location: `assets/icons/ui/` (base), `assets/icons/ui/light/`, `assets/icons/ui/dark/`

#### Wallet Operations
- `wallet.svg` - Wallet icon
- `send.svg` - Send transaction
- `receive.svg` - Receive transaction
- `transactions.svg` - Transaction history
- `balance.svg` - Balance display
- `qr.svg` - QR code
- `address-book.svg` - Address book

#### Transaction States
- `tx-in.svg` - Incoming transaction
- `tx-out.svg` - Outgoing transaction
- `tx-pending.svg` - Pending transaction
- `tx-confirmed.svg` - Confirmed transaction
- `tx-failed.svg` - Failed transaction

#### Network & Sync
- `network.svg` - Network icon
- `network-connected.svg` - Network connected
- `network-disconnected.svg` - Network disconnected
- `sync.svg` - Syncing
- `synced.svg` - Synced
- `peers.svg` - Connected peers
- `status-connected.svg` - Connection status (connected)
- `status-disconnected.svg` - Connection status (disconnected)

#### Security
- `lock.svg` - Locked
- `unlock.svg` - Unlocked
- `key.svg` - Private key
- `shield.svg` - Security shield
- `security.svg` - Security settings
- `backup.svg` - Backup wallet
- `restore.svg` - Restore wallet

#### Staking
- `staking.svg` - Staking icon
- `stake-active.svg` - Active stake
- `stake-inactive.svg` - Inactive stake
- `rewards.svg` - Rewards
- `validator.svg` - Validator

#### Mining
- `mining.svg` - Mining icon
- `cpu.svg` - CPU mining
- `hash.svg` - Hash rate
- `difficulty.svg` - Mining difficulty
- `block.svg` - Block

#### General UI
- `settings.svg` - Settings
- `info.svg` - Information
- `warning.svg` - Warning
- `error.svg` - Error
- `history.svg` - History
- `log.svg` - Logs
- `logo.svg` - App logo
- `app_icon.svg` - App icon
- `memory.svg` - Memory usage
- `mempool.svg` - Mempool
- `disk.svg` - Disk usage

#### PNG Variants (for compatibility)
- `wallet.png` - Wallet (raster)
- `wallet-32.png` - Wallet 32x32
- `wallet-64.png` - Wallet 64x64

## Icon Sizes

Recommended sizes:
- **Tab bar icons**: 24x24 or 28x28
- **Button icons**: 20x20 or 24x24
- **List item icons**: 20x20 or 24x24
- **Header icons**: 24x24 or 28x28
- **Large feature icons**: 48x48 or 64x64

## Color Guidelines

Icons are designed to be colored via the `fill` prop:

```typescript
// Primary actions (brand color)
<SendIcon fill="#FF6B35" />

// Success states
<TxConfirmedIcon fill="#4CAF50" />

// Warning states  
<WarningIcon fill="#FFC107" />

// Error states
<ErrorIcon fill="#F44336" />

// Neutral (theme-based)
<WalletIcon fill={isDark ? "#FFFFFF" : "#000000"} />
```

## Notes

- All SVG icons support dynamic coloring via the `fill` prop
- Theme variants (light/dark) are available for all UI icons
- Icons maintain aspect ratio when scaled
- Use the theme-aware approach for the best user experience
