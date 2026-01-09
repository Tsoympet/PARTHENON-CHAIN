# iOS Native Code

This directory contains the iOS native code for the Drachma Mobile Wallet.

## Structure

```
ios/
├── DrachmaMobileWallet/
│   ├── AppDelegate.h
│   ├── AppDelegate.m
│   ├── Info.plist
│   └── main.m
└── Podfile
```

## Building

To build the iOS app after running `expo prebuild`:

```bash
cd ios
pod install
# Then open .xcworkspace in Xcode
```

## Native Modules

Any native iOS modules required for the app should be placed in this directory.
