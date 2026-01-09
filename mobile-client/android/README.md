# Android Native Code

This directory contains the Android native code for the Drachma Mobile Wallet.

## Structure

```
android/
├── app/
│   └── src/
│       └── main/
│           └── AndroidManifest.xml
├── build.gradle
└── gradle.properties
```

## Building

To build the Android app after running `expo prebuild`:

```bash
cd android
./gradlew assembleDebug
```

## Native Modules

Any native Android modules required for the app should be placed in this directory.
