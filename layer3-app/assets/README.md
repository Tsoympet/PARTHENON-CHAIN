# Layer 3 Assets

This folder contains branding and legal assets loaded by the Drachma Core
Qt application. The GUI references these paths directly; do not rename them
without updating the UI code in `layer3-app/qt/main.cpp`.

- `icons/` holds application and tray icons used across platforms.
- `legal/` contains the EULA bundled with installers and shown on first launch.
- `whitepaper.pdf` should mirror the content of `docs/whitepaper.md` for offline
  consumption inside the app.

To replace assets, drop updated files with the same names. The application will
reload them on the next start without requiring a rebuild.
