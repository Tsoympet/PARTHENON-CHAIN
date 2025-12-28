#include "schnorr.h"
#include "tagged_hash.h"

#include <openssl/crypto.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <array>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {

// RAII helpers for OpenSSL types to avoid leaks.
using ec_group_ptr = std::unique_ptr<EC_GROUP, decltype(&EC_GROUP_free)>;
using ec_point_ptr = std::unique_ptr<EC_POINT, decltype(&EC_POINT_free)>;
using bn_ptr = std::unique_ptr<BIGNUM, decltype(&BN_clear_free)>;
using bn_ctx_ptr = std::unique_ptr<BN_CTX, decltype(&BN_CTX_free)>;

struct bn_ctx_guard {
    explicit bn_ctx_guard(BN_CTX* ctx_in) : ctx(ctx_in) { BN_CTX_start(ctx); }
    ~bn_ctx_guard() {
        if (ctx) {
            BN_CTX_end(ctx);
        }
    }
    BN_CTX* ctx;
};

// Tagged SHA256 as defined in BIP-340.
static std::array<uint8_t, 32> sha256_tagged(const std::string& tag,
                                            const std::vector<uint8_t>& data) {
    std::array<uint8_t, 32> out{};
    uint8_t tag_hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx{};

    // tag_hash = SHA256(tag)
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, tag.data(), tag.size());
    SHA256_Final(tag_hash, &ctx);

    // SHA256(tag_hash || tag_hash || data)
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, tag_hash, sizeof(tag_hash));
    SHA256_Update(&ctx, tag_hash, sizeof(tag_hash));
    if (!data.empty()) {
        SHA256_Update(&ctx, data.data(), data.size());
    }
    SHA256_Final(out.data(), &ctx);
    return out;
}

// Convenience wrapper to create a BIGNUM from raw bytes.
static bn_ptr bn_from_bytes(const uint8_t* buf, size_t len) {
    return bn_ptr(BN_bin2bn(buf, static_cast<int>(len), nullptr), &BN_clear_free);
}

// Reads curve order p and group.
static ec_group_ptr make_secp256k1_group() {
    return ec_group_ptr(EC_GROUP_new_by_curve_name(NID_secp256k1), &EC_GROUP_free);
}

// Serialize BIGNUM to 32-byte big-endian array (padded).
static bool bn_to_fixed_32(const BIGNUM* bn, uint8_t* out32) {
    if (!bn || !out32) {
        return false;
    }
    const int written = BN_bn2binpad(bn, out32, 32);
    return written == 32;
}

// Extract x-only public key from scalar; returns nullptr on error.
static ec_point_ptr compute_public_point(const EC_GROUP* group,
                                         const BIGNUM* scalar,
                                         BN_CTX* ctx) {
    if (!group || !scalar || !ctx) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    ec_point_ptr point(EC_POINT_new(group), &EC_POINT_free);
    if (!point) {
        return point;
    }
    if (EC_POINT_mul(group, point.get(), scalar, nullptr, nullptr, ctx) != 1) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    return point;
}

// Load compressed public key into EC_POINT.
static ec_point_ptr load_public_point(const EC_GROUP* group,
                                      const uint8_t* compressed,
                                      BN_CTX* ctx) {
    if (!group || !compressed || !ctx) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    ec_point_ptr point(EC_POINT_new(group), &EC_POINT_free);
    if (!point) {
        return point;
    }
    if (EC_POINT_oct2point(group, point.get(), compressed, 33, ctx) != 1) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    if (EC_POINT_is_on_curve(group, point.get(), ctx) != 1) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    return point;
}

// Get x and y coordinates of a point.
static bool get_affine_coordinates(const EC_GROUP* group,
                                   const EC_POINT* point,
                                   BIGNUM* x,
                                   BIGNUM* y,
                                   BN_CTX* ctx) {
    return group && point && x && y && ctx &&
           (EC_POINT_get_affine_coordinates_GFp(group, point, x, y, ctx) == 1);
}

// Helper to check if BIGNUM in [1, order-1].
static bool is_valid_secret(const BIGNUM* k, const BIGNUM* order) {
    return k && order && BN_is_zero(k) == 0 && BN_is_negative(k) == 0 &&
           (BN_cmp(k, order) < 0);
}

// Secure random 32 bytes.
static bool fill_random(uint8_t* out32) {
    return out32 && RAND_bytes(out32, 32) == 1;
}

// Compute nonce per BIP-340 using auxiliary randomness.
static bn_ptr compute_bip340_nonce(const BIGNUM* seckey,
                                   const std::array<uint8_t, 32>& pubkey_x,
                                   const uint8_t* msg_hash32,
                                   const BIGNUM* order) {
    uint8_t aux_rand[32]{};
    if (!fill_random(aux_rand)) {
        return bn_ptr(nullptr, &BN_clear_free);
    }

    // t = seckey XOR SHA256_tag("BIP0340/aux", aux_rand)
    std::vector<uint8_t> aux_in(aux_rand, aux_rand + 32);
    const auto aux_hash = tagged_hash("BIP0340/aux", std::span<const uint8_t>(aux_in.data(), aux_in.size()));
    std::array<uint8_t, 32> t{};
    std::array<uint8_t, 32> seckey_bytes{};
    BN_bn2binpad(seckey, seckey_bytes.data(), 32);
    for (size_t i = 0; i < 32; ++i) {
        t[i] = seckey_bytes[i] ^ aux_hash[i];
    }

    // k0 = SHA256_tag("BIP0340/nonce", t || pubkey_x || msg_hash)
    std::vector<uint8_t> nonce_preimage;
    nonce_preimage.reserve(32 + pubkey_x.size() + 32);
    nonce_preimage.insert(nonce_preimage.end(), t.begin(), t.end());
    nonce_preimage.insert(nonce_preimage.end(), pubkey_x.begin(), pubkey_x.end());
    nonce_preimage.insert(nonce_preimage.end(), msg_hash32, msg_hash32 + 32);
    const auto nonce_hash = tagged_hash("BIP0340/nonce", std::span<const uint8_t>(nonce_preimage.data(), nonce_preimage.size()));

    bn_ptr k(bn_from_bytes(nonce_hash.data(), nonce_hash.size()));
    if (!k) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    // k = k0 mod n
    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!ctx) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    if (BN_mod(k.get(), k.get(), order, ctx.get()) != 1) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    if (BN_is_zero(k.get())) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    return k;
}

}

}

// Load compressed public key into EC_POINT.
static ec_point_ptr load_public_point(const EC_GROUP* group,
                                      const uint8_t* compressed,
                                      BN_CTX* ctx) {
    if (!group || !compressed || !ctx) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    ec_point_ptr point(EC_POINT_new(group), &EC_POINT_free);
    if (!point) {
        return point;
    }
    if (EC_POINT_oct2point(group, point.get(), compressed, 33, ctx) != 1) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    if (EC_POINT_is_on_curve(group, point.get(), ctx) != 1) {
        return ec_point_ptr(nullptr, &EC_POINT_free);
    }
    return point;
}

// Get x and y coordinates of a point.
static bool get_affine_coordinates(const EC_GROUP* group,
                                   const EC_POINT* point,
                                   BIGNUM* x,
                                   BIGNUM* y,
                                   BN_CTX* ctx) {
    return group && point && x && y && ctx &&
           (EC_POINT_get_affine_coordinates_GFp(group, point, x, y, ctx) == 1);
}

// Helper to check if BIGNUM in [1, order-1].
static bool is_valid_secret(const BIGNUM* k, const BIGNUM* order) {
    return k && order && BN_is_zero(k) == 0 && BN_is_negative(k) == 0 &&
           (BN_cmp(k, order) < 0);
}

// Secure random 32 bytes.
static bool fill_random(uint8_t* out32) {
    return out32 && RAND_bytes(out32, 32) == 1;
}

// Compute nonce per BIP-340 using auxiliary randomness.
static bn_ptr compute_bip340_nonce(const BIGNUM* seckey,
                                   const std::array<uint8_t, 32>& pubkey_x,
                                   const uint8_t* msg_hash32,
                                   const BIGNUM* order) {
    uint8_t aux_rand[32]{};
    if (!fill_random(aux_rand)) {
        return bn_ptr(nullptr, &BN_clear_free);
    }

    // t = seckey XOR SHA256_tag("BIP0340/aux", aux_rand)
    std::vector<uint8_t> aux_in(aux_rand, aux_rand + 32);
    const auto aux_hash = sha256_tagged("BIP0340/aux", aux_in);
    std::array<uint8_t, 32> t{};
    std::array<uint8_t, 32> seckey_bytes{};
    BN_bn2binpad(seckey, seckey_bytes.data(), 32);
    for (size_t i = 0; i < 32; ++i) {
        t[i] = seckey_bytes[i] ^ aux_hash[i];
    }

    // k0 = SHA256_tag("BIP0340/nonce", t || pubkey_x || msg_hash)
    std::vector<uint8_t> nonce_preimage;
    nonce_preimage.reserve(32 + pubkey_x.size() + 32);
    nonce_preimage.insert(nonce_preimage.end(), t.begin(), t.end());
    nonce_preimage.insert(nonce_preimage.end(), pubkey_x.begin(), pubkey_x.end());
    nonce_preimage.insert(nonce_preimage.end(), msg_hash32, msg_hash32 + 32);
    const auto nonce_hash = sha256_tagged("BIP0340/nonce", nonce_preimage);

    bn_ptr k(bn_from_bytes(nonce_hash.data(), nonce_hash.size()));
    if (!k) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    // k = k0 mod n
    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!ctx) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    if (BN_mod(k.get(), k.get(), order, ctx.get()) != 1) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    if (BN_is_zero(k.get())) {
        return bn_ptr(nullptr, &BN_clear_free);
    }
    return k;
}

}  // namespace

bool schnorr_sign(const uint8_t* private_key,
                  const uint8_t* msg_hash_32,
                  uint8_t* sig_64) {
    if (!private_key || !msg_hash_32 || !sig_64) {
        return false;
    }

    ec_group_ptr group = make_secp256k1_group();
    if (!group) {
        return false;
    }

    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!ctx) {
        return false;
    }
    bn_ctx_guard ctx_guard(ctx.get());

    bn_ptr order(BN_new(), &BN_clear_free);
    if (!order || EC_GROUP_get_order(group.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    bn_ptr seckey = bn_from_bytes(private_key, 32);
    if (!seckey || !is_valid_secret(seckey.get(), order.get())) {
        return false;
    }

    bn_ptr px(BN_new(), &BN_clear_free);
    bn_ptr py(BN_new(), &BN_clear_free);
    if (!px || !py) {
        return false;
    }

    // Derive public key and enforce even Y by negating secret if needed.
    ec_point_ptr pub_point = compute_public_point(group.get(), seckey.get(), ctx.get());
    if (!pub_point) {
        return false;
    }
    if (get_affine_coordinates(group.get(), pub_point.get(), px.get(), py.get(), ctx.get()) != 1) {
        return false;
    }
    if (BN_is_odd(py.get())) {
        // seckey = n - seckey; pub_point = -pub_point
        if (BN_sub(seckey.get(), order.get(), seckey.get()) != 1) {
            return false;
        }
        EC_POINT_invert(group.get(), pub_point.get(), ctx.get());
        if (get_affine_coordinates(group.get(), pub_point.get(), px.get(), py.get(), ctx.get()) != 1) {
            return false;
        }
        EC_POINT_invert(group.get(), pub_point.get(), ctx.get());
        if (get_affine_coordinates(group.get(), pub_point.get(), px.get(), py.get(), ctx.get()) != 1) {
            return false;
        }
    }

    std::array<uint8_t, 32> pub_x_bytes{};
    if (!bn_to_fixed_32(px.get(), pub_x_bytes.data())) {
        return false;
    }

    // Compute deterministic nonce.
    bn_ptr k = compute_bip340_nonce(seckey.get(), pub_x_bytes, msg_hash_32, order.get());
    if (!k) {
        return false;
    }

    }

    // Compute deterministic nonce.
    bn_ptr k = compute_bip340_nonce(seckey.get(), pub_x_bytes, msg_hash_32, order.get());
    if (!k) {
        return false;
    }

    std::array<uint8_t, 32> pub_x_bytes{};
    if (!bn_to_fixed_32(px.get(), pub_x_bytes.data())) {
        return false;
    }

    // Compute deterministic nonce.
    bn_ptr k = compute_bip340_nonce(seckey.get(), pub_x_bytes, msg_hash_32, order.get());
    if (!k) {
        return false;
    }

    // Compute R = k*G and ensure even Y, possibly negating k.
    bn_ptr rx(BN_new(), &BN_clear_free);
    bn_ptr ry(BN_new(), &BN_clear_free);
    if (!rx || !ry) {
        return false;
    }
    ec_point_ptr r_point = compute_public_point(group.get(), k.get(), ctx.get());
    if (!r_point) {
        return false;
    }
    if (get_affine_coordinates(group.get(), r_point.get(), rx.get(), ry.get(), ctx.get()) != 1) {
        return false;
    }
    if (BN_is_odd(ry.get())) {
        // k = n - k, R = -R to force even Y
        if (BN_sub(k.get(), order.get(), k.get()) != 1) {
            return false;
        }
        EC_POINT_invert(group.get(), r_point.get(), ctx.get());
        if (get_affine_coordinates(group.get(), r_point.get(), rx.get(), ry.get(), ctx.get()) != 1) {
            return false;
        }
    }

    std::array<uint8_t, 32> r_bytes{};
    if (!bn_to_fixed_32(rx.get(), r_bytes.data())) {
        return false;
    }

    // Compute challenge e = int(hash("BIP0340/challenge", r || pub_x || msg)) mod n
    std::vector<uint8_t> challenge_preimage;
    challenge_preimage.reserve(32 + 32 + 32);
    challenge_preimage.insert(challenge_preimage.end(), r_bytes.begin(), r_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), pub_x_bytes.begin(), pub_x_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), msg_hash_32, msg_hash_32 + 32);
    const auto challenge_hash = tagged_hash("BIP0340/challenge", std::span<const uint8_t>(challenge_preimage.data(), challenge_preimage.size()));
    const auto challenge_hash = sha256_tagged("BIP0340/challenge", challenge_preimage);
    bn_ptr e(bn_from_bytes(challenge_hash.data(), challenge_hash.size()));
    if (!e) {
        return false;
    }
    if (BN_mod(e.get(), e.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // s = (k + e*seckey) mod n
    bn_ptr s(BN_new(), &BN_clear_free);
    if (!s) {
        return false;
    }
    bn_ptr tmp(BN_new(), &BN_clear_free);
    if (!tmp) {
        return false;
    }
    if (BN_mod_mul(tmp.get(), e.get(), seckey.get(), order.get(), ctx.get()) != 1) {
        return false;
    }
    if (BN_mod_add(s.get(), k.get(), tmp.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // Encode signature r || s
    if (!bn_to_fixed_32(rx.get(), sig_64) || !bn_to_fixed_32(s.get(), sig_64 + 32)) {
        return false;
    }

    return true;
}

bool schnorr_verify(const uint8_t* public_key_33_compressed,
                    const uint8_t* msg_hash_32,
                    const uint8_t* sig_64) {
    if (!public_key_33_compressed || !msg_hash_32 || !sig_64) {
        return false;
    }

    ec_group_ptr group = make_secp256k1_group();
    if (!group) {
        return false;
    }

    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!ctx) {
        return false;
    }
    bn_ctx_guard ctx_guard(ctx.get());

    // Parse r and s
    bn_ptr r = bn_from_bytes(sig_64, 32);
    bn_ptr s = bn_from_bytes(sig_64 + 32, 32);
    if (!r || !s) {
        return false;
    }

    bn_ptr order(BN_new(), &BN_clear_free);
    if (!order || EC_GROUP_get_order(group.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // Check field prime p for r < p
    bn_ptr p(BN_new(), &BN_clear_free);
    bn_ptr a(BN_new(), &BN_clear_free);
    bn_ptr b(BN_new(), &BN_clear_free);
    if (!p || !a || !b ||
        EC_GROUP_get_curve_GFp(group.get(), p.get(), a.get(), b.get(), ctx.get()) != 1) {
        return false;
    }
    if (BN_is_negative(r.get()) == 1 || BN_cmp(r.get(), p.get()) >= 0) {
        return false;
    }
    if (BN_is_negative(s.get()) == 1 || BN_cmp(s.get(), order.get()) >= 0) {
        return false;
    }

    // Load public key
    ec_point_ptr pub_point = load_public_point(group.get(), public_key_33_compressed, ctx.get());
    if (!pub_point) {
        return false;
    }
    bn_ptr px(BN_new(), &BN_clear_free);
    bn_ptr py(BN_new(), &BN_clear_free);
    if (!px || !py ||
        get_affine_coordinates(group.get(), pub_point.get(), px.get(), py.get(), ctx.get()) != 1) {
        return false;
    }

    std::array<uint8_t, 32> pub_x_bytes{};
    if (!bn_to_fixed_32(px.get(), pub_x_bytes.data())) {
        return false;
    }

    // Compute challenge hash
    std::array<uint8_t, 32> r_bytes{};
    if (!bn_to_fixed_32(r.get(), r_bytes.data())) {
        return false;
    }
    std::vector<uint8_t> challenge_preimage;
    challenge_preimage.reserve(32 + 32 + 32);
    challenge_preimage.insert(challenge_preimage.end(), r_bytes.begin(), r_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), pub_x_bytes.begin(), pub_x_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), msg_hash_32, msg_hash_32 + 32);
    const auto challenge_hash = tagged_hash("BIP0340/challenge", std::span<const uint8_t>(challenge_preimage.data(), challenge_preimage.size()));
    bn_ptr e(bn_from_bytes(challenge_hash.data(), challenge_hash.size()));
    if (!e) {
        return false;
    }
    if (BN_mod(e.get(), e.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // R = s*G - e*P
    ec_point_ptr r_point(EC_POINT_new(group.get()), &EC_POINT_free);
    ec_point_ptr sG(EC_POINT_new(group.get()), &EC_POINT_free);
    ec_point_ptr eP(EC_POINT_new(group.get()), &EC_POINT_free);
    if (!r_point || !sG || !eP) {
        return false;
    }
    if (EC_POINT_mul(group.get(), sG.get(), s.get(), nullptr, nullptr, ctx.get()) != 1) {
        return false;
    }
    if (EC_POINT_mul(group.get(), eP.get(), nullptr, pub_point.get(), e.get(), ctx.get()) != 1) {
        return false;
    }
    EC_POINT_invert(group.get(), eP.get(), ctx.get());
    if (EC_POINT_add(group.get(), r_point.get(), sG.get(), eP.get(), ctx.get()) != 1) {
        return false;
    }

    if (EC_POINT_is_at_infinity(group.get(), r_point.get()) == 1) {
        return false;
    }

    bn_ptr rx(BN_new(), &BN_clear_free);
    bn_ptr ry(BN_new(), &BN_clear_free);
    if (!rx || !ry ||
        get_affine_coordinates(group.get(), r_point.get(), rx.get(), ry.get(), ctx.get()) != 1) {
        return false;
    }

    // Check even Y and x == r
    std::array<uint8_t, 32> rx_bytes{};
    if (!bn_to_fixed_32(rx.get(), rx_bytes.data())) {
        return false;
    }
    const bool y_even = BN_is_odd(ry.get()) == 0;
    const bool x_matches = CRYPTO_memcmp(rx_bytes.data(), r_bytes.data(), rx_bytes.size()) == 0;

    return y_even && x_matches;
}

}

bool schnorr_verify(const uint8_t* public_key_33_compressed,
                    const uint8_t* msg_hash_32,
                    const uint8_t* sig_64) {
    if (!public_key_33_compressed || !msg_hash_32 || !sig_64) {
        return false;
    }

    ec_group_ptr group = make_secp256k1_group();
    if (!group) {
        return false;
    }

    bn_ctx_ptr ctx(BN_CTX_new(), &BN_CTX_free);
    if (!ctx) {
        return false;
    }
    bn_ctx_guard ctx_guard(ctx.get());

    // Parse r and s
    bn_ptr r = bn_from_bytes(sig_64, 32);
    bn_ptr s = bn_from_bytes(sig_64 + 32, 32);
    if (!r || !s) {
        return false;
    }

    bn_ptr order(BN_new(), &BN_clear_free);
    if (!order || EC_GROUP_get_order(group.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // Check field prime p for r < p
    bn_ptr p(BN_new(), &BN_clear_free);
    bn_ptr a(BN_new(), &BN_clear_free);
    bn_ptr b(BN_new(), &BN_clear_free);
    if (!p || !a || !b ||
        EC_GROUP_get_curve_GFp(group.get(), p.get(), a.get(), b.get(), ctx.get()) != 1) {
        return false;
    }
    if (BN_is_negative(r.get()) == 1 || BN_cmp(r.get(), p.get()) >= 0) {
        return false;
    }
    if (BN_is_negative(s.get()) == 1 || BN_cmp(s.get(), order.get()) >= 0) {
        return false;
    }

    // Load public key
    ec_point_ptr pub_point = load_public_point(group.get(), public_key_33_compressed, ctx.get());
    if (!pub_point) {
        return false;
    }
    bn_ptr px(BN_new(), &BN_clear_free);
    bn_ptr py(BN_new(), &BN_clear_free);
    if (!px || !py ||
        get_affine_coordinates(group.get(), pub_point.get(), px.get(), py.get(), ctx.get()) != 1) {
        return false;
    }

    std::array<uint8_t, 32> pub_x_bytes{};
    if (!bn_to_fixed_32(px.get(), pub_x_bytes.data())) {
        return false;
    }

    // Compute challenge hash
    std::array<uint8_t, 32> r_bytes{};
    if (!bn_to_fixed_32(r.get(), r_bytes.data())) {
        return false;
    }
    std::vector<uint8_t> challenge_preimage;
    challenge_preimage.reserve(32 + 32 + 32);
    challenge_preimage.insert(challenge_preimage.end(), r_bytes.begin(), r_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), pub_x_bytes.begin(), pub_x_bytes.end());
    challenge_preimage.insert(challenge_preimage.end(), msg_hash_32, msg_hash_32 + 32);
    const auto challenge_hash = tagged_hash("BIP0340/challenge", std::span<const uint8_t>(challenge_preimage.data(), challenge_preimage.size()));
    const auto challenge_hash = sha256_tagged("BIP0340/challenge", challenge_preimage);
    bn_ptr e(bn_from_bytes(challenge_hash.data(), challenge_hash.size()));
    if (!e) {
        return false;
    }
    if (BN_mod(e.get(), e.get(), order.get(), ctx.get()) != 1) {
        return false;
    }

    // R = s*G - e*P
    ec_point_ptr r_point(EC_POINT_new(group.get()), &EC_POINT_free);
    ec_point_ptr sG(EC_POINT_new(group.get()), &EC_POINT_free);
    ec_point_ptr eP(EC_POINT_new(group.get()), &EC_POINT_free);
    if (!r_point || !sG || !eP) {
        return false;
    }
    if (EC_POINT_mul(group.get(), sG.get(), s.get(), nullptr, nullptr, ctx.get()) != 1) {
        return false;
    }
    if (EC_POINT_mul(group.get(), eP.get(), nullptr, pub_point.get(), e.get(), ctx.get()) != 1) {
        return false;
    }
    EC_POINT_invert(group.get(), eP.get(), ctx.get());
    if (EC_POINT_add(group.get(), r_point.get(), sG.get(), eP.get(), ctx.get()) != 1) {
        return false;
    }

    if (EC_POINT_is_at_infinity(group.get(), r_point.get()) == 1) {
        return false;
    }

    bn_ptr rx(BN_new(), &BN_clear_free);
    bn_ptr ry(BN_new(), &BN_clear_free);
    if (!rx || !ry ||
        get_affine_coordinates(group.get(), r_point.get(), rx.get(), ry.get(), ctx.get()) != 1) {
        return false;
    }

    // Check even Y and x == r
    std::array<uint8_t, 32> rx_bytes{};
    if (!bn_to_fixed_32(rx.get(), rx_bytes.data())) {
        return false;
    }
    const bool y_even = BN_is_odd(ry.get()) == 0;
    const bool x_matches = CRYPTO_memcmp(rx_bytes.data(), r_bytes.data(), rx_bytes.size()) == 0;

    return y_even && x_matches;
}

// -----------------------------------------------------------------------------
// Testing guidance:
// - Use BIP-340 published test vectors. For each vector, feed the secret key,
//   message, and expected signature into schnorr_sign and schnorr_verify.
// - Verify sign() output matches known signatures and that tampering with
//   a single bit in the signature or message makes schnorr_verify return false.
// - Test invalid inputs: zero secret keys, out-of-range scalars, malformed
//   compressed public keys, and ensure schnorr_sign/verify return false.
// Example snippet (pseudo-code):
//   std::array<uint8_t,32> msg = {...};
//   std::array<uint8_t,32> seckey = {...};
//   std::array<uint8_t,33> pubkey = {...};
//   std::array<uint8_t,64> sig{};
//   assert(schnorr_sign(seckey.data(), msg.data(), sig.data()));
//   assert(schnorr_verify(pubkey.data(), msg.data(), sig.data()));
// -----------------------------------------------------------------------------
