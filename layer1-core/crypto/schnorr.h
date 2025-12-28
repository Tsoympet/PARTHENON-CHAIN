#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// BIP-340 Schnorr signature API for secp256k1 using OpenSSL.
// All buffers are expected to be exact size: private key 32 bytes,
// message hash 32 bytes, compressed public key 33 bytes, signature 64 bytes.
// Functions return true on success and false on any failure.
bool schnorr_sign(const uint8_t* private_key,
                  const uint8_t* msg_hash_32,
                  uint8_t* sig_64);

bool schnorr_verify(const uint8_t* public_key_33_compressed,
                    const uint8_t* msg_hash_32,
                    const uint8_t* sig_64);

// Example usage (see schnorr.cpp for details and testing suggestions):
//   std::array<uint8_t,32> priv{}; // load secret key
//   std::array<uint8_t,32> msg_hash{}; // 32-byte message digest
//   std::array<uint8_t,64> sig{};
//   if (!schnorr_sign(priv.data(), msg_hash.data(), sig.data())) { /* handle error */ }
//   // derive/compress public key separately
//   if (!schnorr_verify(pubkey.data(), msg_hash.data(), sig.data())) { /* invalid */ }
