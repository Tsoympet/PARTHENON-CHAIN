# Android Native Code

This directory contains the Android native code for the Drachma Mobile Wallet.

## Structure

```
android/
├── app/
│   └── src/
│       └── main/
│           ├── AndroidManifest.xml
│           ├── java/
│           └── res/
├── build.gradle
└── gradle.properties
```

## Building

To build the Android app:

```bash
cd android
./gradlew assembleDebug
```

## Native Modules

Any native Android modules required for the app should be placed in this directory.
