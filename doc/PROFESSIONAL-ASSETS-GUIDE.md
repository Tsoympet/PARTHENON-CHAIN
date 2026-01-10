# PARTHENON CHAIN Professional Assets Guide

**Version:** 2.0  
**Last Updated:** January 10, 2026  
**Status:** Professional Icons Available

## Overview

PARTHENON CHAIN now includes professional-grade icon assets based on Classical Greek design principles. This guide documents the available assets, their usage, and design philosophy.

## Design Philosophy

**Theme:** Classical Greece - Timeless, Order, Value  
**Visual DNA:** Marble, Bronze, Silver, Obsidian  
**Aesthetic:** Architectural elegance meets blockchain permanence

### Color Palette

**Primary Materials:**
- **Marble:** `#FAFAF8` to `#D4D4C8` - Columns and structure
- **Bronze:** `#D4AF37` to `#996515` - Accents and highlights
- **Silver:** `#E8E8E8` to `#A0A0A0` - Metallic details
- **Obsidian:** `#2C2C2E` to `#1C1C1E` - Dark backgrounds

**Asset-Specific Colors:**
- **TLN (Talanton/Bronze):** Bronze tones (`#CD7F32`)
- **DRM (Drachma/Silver):** Silver tones (`#C0C0C0`)
- **OBL (Obolos/Aegean Blue):** Blue tones (`#4A90B8`)

## Available Icon Sets

### Core Application Icons

Located in `/assets/core-icons/`

#### Standard Icons (Functional Placeholders)
- `app-icon.svg` - 128x128 - Basic Parthenon silhouette
- `splash-icon.svg` - 128x128 - Simple splash screen
- `tray-icon.svg` - 64x64 - Minimalist system tray

#### Professional Icons (Production Ready) ✨ NEW
- `app-icon-pro.svg` - 256x256 - High-detail application icon
- `splash-icon-pro.svg` - 512x512 - Full splash screen with effects
- `tray-icon-pro.svg` - 64x64 - Refined system tray icon

### Asset Icons

Located in `/assets/icons/`

#### Standard Asset Icons
- `asset-tln.svg` - TLN coin icon
- `asset-drm.svg` - DRM coin icon
- `asset-obl.svg` - OBL coin icon

#### Professional Asset Icons ✨ NEW
- `asset-tln-pro.svg` - 128x128 - Bronze-themed TLN coin
- `asset-drm-pro.svg` - 128x128 - Silver-themed DRM coin
- `asset-obl-pro.svg` - 128x128 - Aegean Blue-themed OBL coin

## Icon Features

### Professional App Icon (`app-icon-pro.svg`)

**Size:** 256x256 pixels  
**Features:**
- Five detailed Doric columns with fluting
- Bronze pediment (triangular top)
- Marble gradients with realistic texture
- Silver highlights on capitals and bases
- Obsidian background with radial gradient
- Shadow effects for depth
- "PARTHENON" text in classical serif font

**Usage:** Primary application icon for desktop environments

### Professional Splash Icon (`splash-icon-pro.svg`)

**Size:** 512x512 pixels  
**Features:**
- Seven monumental columns with architectural detail
- Entablature and frieze with decorative elements
- Greek key patterns in pediment
- Glow and shadow effects
- Full "PARTHENON CHAIN" branding
- "Built to Endure" tagline
- Subtle blockchain element in background

**Usage:** Application splash screen, loading screen

### Professional Tray Icon (`tray-icon-pro.svg`)

**Size:** 64x64 pixels  
**Features:**
- Simplified three-column design
- Clear silhouette at small sizes
- High contrast for visibility
- Minimal detail for clarity

**Usage:** System tray, notification area, small icons

### Professional Asset Icons

**Size:** 128x128 pixels  
**Common Features:**
- Coin-like circular design
- Radial gradients for dimensionality
- Shadow effects for depth
- Asset-specific color theming
- Classical typography
- Greek decorative patterns

**TLN Icon:**
- Bronze tones reflecting talent (large denomination)
- Greek key pattern accents
- "TALANTON" subtitle

**DRM Icon:**
- Silver tones for primary currency
- Owl silhouette (Athena's symbol of wisdom)
- "DRACHMA" subtitle

**OBL Icon:**
- Aegean Blue for settlement operations
- Wave pattern (Mediterranean sea)
- "OBOLOS" subtitle

## Migration Guide

### From Standard to Professional Icons

To use the professional icons in your application:

#### Option 1: Replace Files (Breaking Change)
```bash
# Backup originals
cp assets/core-icons/app-icon.svg assets/core-icons/app-icon-old.svg

# Replace with professional versions
cp assets/core-icons/app-icon-pro.svg assets/core-icons/app-icon.svg
cp assets/core-icons/splash-icon-pro.svg assets/core-icons/splash-icon.svg
cp assets/core-icons/tray-icon-pro.svg assets/core-icons/tray-icon.svg
```

#### Option 2: Update References (Non-Breaking)
Update your application code to reference the `-pro` variants:

```cpp
// Before
QIcon appIcon(":/assets/core-icons/app-icon.svg");

// After
QIcon appIcon(":/assets/core-icons/app-icon-pro.svg");
```

### Asset Icons Migration

```bash
# Replace asset icons
cp assets/icons/asset-tln-pro.svg assets/icons/asset-tln.svg
cp assets/icons/asset-drm-pro.svg assets/icons/asset-drm.svg
cp assets/icons/asset-obl-pro.svg assets/icons/asset-obl.svg
```

## File Formats

### SVG (Source Format)
- **Scalable** - Can be rendered at any size
- **Editable** - Can be modified with vector editors
- **Small** - Compact file size
- **Quality** - No loss at any resolution

### Recommended Rasterization

For platforms requiring raster formats:

```bash
# Install Inkscape or ImageMagick
sudo apt-get install inkscape

# Convert to PNG at various sizes
inkscape --export-type=png \
    --export-width=512 \
    --export-filename=app-icon-512.png \
    assets/core-icons/app-icon-pro.svg

# Create icon set (macOS)
iconutil -c icns AppIcon.iconset

# Create icon set (Windows)
convert app-icon-{16,32,48,64,128,256}.png app-icon.ico
```

## Platform-Specific Requirements

### Linux
- **Desktop File:** Use 128x128 or 256x256 PNG
- **Tray Icon:** Use 22x22 or 24x24 PNG (monochrome variant recommended)
- **Location:** `/usr/share/icons/hicolor/`

### macOS
- **App Icon:** `.icns` file with multiple resolutions (16-1024px)
- **DMG Background:** Use splash icon at 2x resolution
- **Retina Support:** Provide @2x variants

### Windows
- **App Icon:** `.ico` file with 16, 32, 48, 256 pixel versions
- **Installer:** Use 256x256 for setup wizard
- **Taskbar:** Windows will scale automatically

## Design Guidelines

### When to Use Which Icon

**App Icon (256x256):**
- Application launchers
- Desktop shortcuts
- File associations
- About dialogs

**Splash Icon (512x512):**
- Application startup screen
- Welcome screen
- Marketing materials
- Website hero images

**Tray Icon (64x64):**
- System tray/notification area
- Menu bar (macOS)
- Status indicators
- Minimized state

**Asset Icons (128x128):**
- Wallet balance display
- Transaction history
- Asset selection dialogs
- Portfolio views

## Customization

### Editing Icons

Professional icons are SVG and can be edited with:
- **Inkscape** (Free, open source)
- **Adobe Illustrator** (Commercial)
- **Figma** (Web-based, free tier)

### Maintaining Brand Consistency

When customizing:
1. Preserve the Parthenon architectural motif
2. Maintain the color palette (Marble, Bronze, Silver, Obsidian)
3. Use Classical serif fonts (Georgia, Garamond, Trajan)
4. Keep Greek decorative elements
5. Ensure readability at all sizes

### Creating Variants

```svg
<!-- Example: Dark Mode Variant -->
<defs>
  <linearGradient id="darkMarble">
    <stop offset="0%" style="stop-color:#4A4A4C"/>
    <stop offset="100%" style="stop-color:#2C2C2E"/>
  </linearGradient>
</defs>
```

## Quality Checklist

Before using icons in production:

- [ ] Icons display clearly at target size
- [ ] Colors match brand guidelines
- [ ] No pixelation or artifacts
- [ ] Readable on both light and dark backgrounds
- [ ] Recognizable when scaled down
- [ ] Consistent style across all icons
- [ ] Proper file naming convention
- [ ] SVG files are optimized (no unnecessary elements)
- [ ] Tested on target platforms

## File Inventory

### Current Assets
- **Core Icons:** 6 files (3 standard + 3 professional)
- **Asset Icons:** 6 files (3 standard + 3 professional)
- **UI Icons:** 160+ files (existing functional set)
- **Total:** 172+ icon files

### Recommended Production Set
- Use professional versions for all public releases
- Keep standard versions for development/testing
- Maintain both for backward compatibility

## Future Enhancements

### Planned Additions
- [ ] Animated SVG variants for loading states
- [ ] Dark mode optimized variants
- [ ] High-DPI @3x variants for latest displays
- [ ] Iconography for Layer 2/3 features
- [ ] Platform-specific optimizations

### Community Contributions
We welcome contributions of:
- Platform-specific icon sets
- Themed variants (dark, light, high-contrast)
- Localized versions
- Accessibility improvements

## Resources

### Design Tools
- **Inkscape:** https://inkscape.org/ (Free SVG editor)
- **SVGOMG:** https://jakearchibald.github.io/svgomg/ (SVG optimizer)
- **RealFaviconGenerator:** https://realfavicongenerator.net/ (Multi-platform icons)

### Inspiration
- **Classical Greek Architecture:** Parthenon, Temple of Olympian Zeus
- **Ancient Coins:** Athenian Tetradrachm, Corinthian Stater
- **Typography:** Trajan, Optima, Perpetua (Classical serif fonts)

## Support

**Questions about assets?**
- GitHub Issues: https://github.com/Tsoympet/PARTHENON-CHAIN/issues
- Discussions: https://github.com/Tsoympet/PARTHENON-CHAIN/discussions

**Design Contributions:**
- Submit PR with new icons
- Include usage examples
- Follow design guidelines

---

**Remember:** Icons are the face of PARTHENON CHAIN. Professional, consistent iconography builds trust and reflects our commitment to quality and timeless value.
