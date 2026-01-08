# Mobile Client Tests

This directory contains all test files for the Drachma Mobile Wallet.

## Test Structure

```
__tests__/
├── components.test.tsx    # Component tests
├── utils.test.ts          # Utility function tests
├── crypto.test.ts         # Cryptographic service tests
├── screens.test.tsx       # Screen component tests
└── integration.test.tsx   # Integration tests
```

## Running Tests

```bash
# Run all tests
npm test

# Run tests in watch mode
npm test -- --watch

# Run tests with coverage
npm test -- --coverage
```

## Writing Tests

Follow these guidelines when writing tests:

1. Use descriptive test names
2. Test both success and failure cases
3. Mock external dependencies
4. Aim for high code coverage
5. Keep tests isolated and independent
