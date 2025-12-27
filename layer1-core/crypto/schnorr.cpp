#include "schnorr.h"
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <stdexcept>
#include <vector>

namespace {

// Tagged hash for Schnorr following BIP-340
static void TaggedHashBIP340(const std::vector<uint8_t>& msg, std::array<uint8_t,32>& out)
{
    const std::string tag = "BIP0340/challenge";
    uint8_t taghash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, tag.data(), tag.size());
    SHA256_Final(taghash, &ctx);

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, taghash, sizeof(taghash));
    SHA256_Update(&ctx, taghash, sizeof(taghash));
    SHA256_Update(&ctx, msg.data(), msg.size());
    SHA256_Final(out.data(), &ctx);
}

static bool LoadPubKey(const std::array<uint8_t,32>& xonly, EC_GROUP* group, EC_POINT* point)
{
    // Recover even Y from x-only key as per BIP340
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) return false;
    BN_CTX_start(ctx);
    BIGNUM* x = BN_bin2bn(xonly.data(), xonly.size(), nullptr);
    if (!x) { BN_CTX_end(ctx); BN_CTX_free(ctx); return false; }
    EC_POINT* p = EC_POINT_new(group);
    bool ok = EC_POINT_set_compressed_coordinates_GFp(group, p, x, 0, ctx);
    if (ok) {
        if (EC_POINT_is_on_curve(group, p, ctx) != 1) ok = false;
        if (ok) {
            EC_POINT_copy(point, p);
        }
    }
    EC_POINT_free(p);
    BN_free(x);
    BN_CTX_end(ctx);
    BN_CTX_free(ctx);
    return ok;
}

} // namespace

bool VerifySchnorr(
    const std::array<uint8_t,32>& pubkey,
    const std::array<uint8_t,64>& sig,
    const std::vector<uint8_t>& msg)
{
    // Signature structure: r(32) || s(32)
    std::array<uint8_t,32> rBytes{};
    std::array<uint8_t,32> sBytes{};
    std::copy(sig.begin(), sig.begin() + 32, rBytes.begin());
    std::copy(sig.begin() + 32, sig.end(), sBytes.begin());

    BIGNUM* r = BN_bin2bn(rBytes.data(), rBytes.size(), nullptr);
    BIGNUM* s = BN_bin2bn(sBytes.data(), sBytes.size(), nullptr);
    if (!r || !s) { BN_free(r); BN_free(s); return false; }

    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
    if (!group) { BN_free(r); BN_free(s); return false; }
    EC_POINT* P = EC_POINT_new(group);
    if (!P) { BN_free(r); BN_free(s); EC_GROUP_free(group); return false; }

    if (!LoadPubKey(pubkey, group, P)) {
        BN_free(r); BN_free(s); EC_POINT_free(P); EC_GROUP_free(group); return false; }

    BN_CTX* bnctx = BN_CTX_new();
    if (!bnctx) { BN_free(r); BN_free(s); EC_POINT_free(P); EC_GROUP_free(group); return false; }

    // Compute challenge e = int(hash(r||pubkey||msg)) mod n
    const int order_bits = EC_GROUP_get_degree(group);
    BIGNUM* n = BN_new();
    EC_GROUP_get_order(group, n, bnctx);

    std::vector<uint8_t> preimage;
    preimage.insert(preimage.end(), rBytes.begin(), rBytes.end());
    preimage.insert(preimage.end(), pubkey.begin(), pubkey.end());
    preimage.insert(preimage.end(), msg.begin(), msg.end());
    std::array<uint8_t,32> eBytes{};
    TaggedHashBIP340(preimage, eBytes);
    BIGNUM* e = BN_bin2bn(eBytes.data(), eBytes.size(), nullptr);
    BN_mod(e, e, n, bnctx);

    // s*G = R + e*P => sG - eP ?= R with x = r and even y
    EC_POINT* sG = EC_POINT_new(group);
    EC_POINT_mul(group, sG, s, nullptr, nullptr, bnctx);

    EC_POINT* eP = EC_POINT_new(group);
    EC_POINT_mul(group, eP, nullptr, P, e, bnctx);

    EC_POINT* R = EC_POINT_new(group);
    EC_POINT_invert(group, eP, bnctx);
    EC_POINT_add(group, R, sG, eP, bnctx);

    // Verify R not at infinity
    if (EC_POINT_is_at_infinity(group, R)) {
        BN_free(r); BN_free(s); BN_free(e); BN_free(n);
        EC_POINT_free(P); EC_POINT_free(sG); EC_POINT_free(eP); EC_POINT_free(R); EC_GROUP_free(group); BN_CTX_free(bnctx);
        return false;
    }

    // Check x coordinate == r and R has even y
    BIGNUM* xR = BN_new();
    BIGNUM* yR = BN_new();
    EC_POINT_get_affine_coordinates_GFp(group, R, xR, yR, bnctx);
    bool y_even = BN_is_bit_set(yR, 0) == 0;
    std::array<uint8_t,32> xBytes{};
    BN_bn2binpad(xR, xBytes.data(), xBytes.size());

    bool match = y_even && (std::equal(xBytes.begin(), xBytes.end(), rBytes.begin()));

    BN_free(r); BN_free(s); BN_free(e); BN_free(n);
    BN_free(xR); BN_free(yR);
    EC_POINT_free(P); EC_POINT_free(sG); EC_POINT_free(eP); EC_POINT_free(R);
    EC_GROUP_free(group);
    BN_CTX_free(bnctx);
    return match;
}
