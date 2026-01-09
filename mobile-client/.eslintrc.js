module.exports = {
  root: true,
  extends: '@react-native',
  parser: '@typescript-eslint/parser',
  plugins: ['@typescript-eslint', 'react', 'react-native'],
  parserOptions: {
    ecmaFeatures: {
      jsx: true,
    },
    ecmaVersion: 2021,
    sourceType: 'module',
  },
  env: {
    'react-native/react-native': true,
  },
  rules: {
    // TypeScript rules
    '@typescript-eslint/no-unused-vars': ['warn', {argsIgnorePattern: '^_'}],
    '@typescript-eslint/no-explicit-any': 'warn',
    
    // React rules
    'react/prop-types': 'off', // TypeScript handles this
    'react-native/no-inline-styles': 'warn',
    'react-native/no-unused-styles': 'warn',
    
    // General rules
    'no-console': ['warn', {allow: ['warn', 'error']}],
    'prefer-const': 'warn',
    'no-var': 'error',
  },
};
