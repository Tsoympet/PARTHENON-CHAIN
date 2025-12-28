#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

// BIP-340 Schnorr signature API for secp256k1 using OpenSSL.
// All buffers are expected to be exact size: private key 32 bytes,
// message hash 32 bytes, compressed public key 33 bytes, signature 64 bytes.
// Functions return true on success and false on any failure.
bool schnorr_sign(const uint8_t* private_key,
                  const uint8_t* msg_hash_32,
                  uint8_t* sig_64);

// Deterministic signing helper that accepts caller-supplied 32-byte auxiliary
// randomness (as defined in BIP-340). When aux_rand_32 is nullptr, falls back
// to secure randomness. Intended for test vectors and reproducibility.
bool schnorr_sign_with_aux(const uint8_t* private_key,
                           const uint8_t* msg_hash_32,
                           const uint8_t* aux_rand_32,
                           uint8_t* sig_64);

bool schnorr_verify(const uint8_t* public_key_33_compressed,
                    const uint8_t* msg_hash_32,
                    const uint8_t* sig_64);

// Batch verification for multiple independent signatures. All vector inputs
// must be identical in length. Returns true if every signature validates.
// This routine mixes random coefficients to prevent attackers from crafting
// degenerate batches and falls back to scalar verification when inputs are
// inconsistent.
bool schnorr_batch_verify(const std::vector<std::array<uint8_t, 33>>& pubkeys,
                          const std::vector<std::array<uint8_t, 32>>& msg_hashes,
                          const std::vector<std::array<uint8_t, 64>>& signatures);

// Convenience wrapper used by legacy call sites that provide an x-only public
// key, raw message bytes, and a 64-byte signature. When the message is already
// a 32-byte digest (as with BIP-340 test vectors), the digest is used as-is;
// otherwise the message is hashed with SHA-256 before verifying with the
// lower-level schnorr_verify routine.
bool VerifySchnorr(const std::array<uint8_t, 32>& pubkey_x,
                   const std::array<uint8_t, 64>& sig,
                   const std::vector<uint8_t>& msg);

// Example usage (see schnorr.cpp for details and testing suggestions):
//   std::array<uint8_t,32> priv{}; // load secret key
//   std::array<uint8_t,32> msg_hash{}; // 32-byte message digest
//   std::array<uint8_t,64> sig{};
//   if (!schnorr_sign(priv.data(), msg_hash.data(), sig.data())) { /* handle error */ }
//   // derive/compress public key separately
//   if (!schnorr_verify(pubkey.data(), msg_hash.data(), sig.data())) { /* invalid */ }
