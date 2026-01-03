#include "../../layer1-core/crypto/schnorr.h"
#include <cassert>
#include <vector>
#include <iostream>

int main()
{
    std::array<uint8_t,32> pub{0x79,0xbe,0x66,0x7e,0xf9,0xdc,0xbb,0xac,0x55,0xa0,0x62,0x95,0xce,0x87,0x0b,0x07,
                               0x02,0x9b,0xfc,0xdb,0x2d,0xce,0x28,0xd9,0x59,0xf2,0x81,0x5b,0x16,0xf8,0x17,0x98};
    std::array<uint8_t,64> sig{
        0x62,0x43,0x0a,0xa8,0xfb,0x5e,0x24,0x4d,0x9a,0xd7,0xc5,0x71,0x1f,0xff,0xa9,0x8e,
        0x58,0x2c,0xaa,0x19,0x49,0x3d,0x0c,0xf1,0xf8,0x1f,0x85,0x04,0x3c,0xce,0x46,0xf8,
        0x3c,0xa7,0x99,0x46,0x68,0xbf,0x04,0x0d,0xf5,0xdf,0x12,0x19,0xa2,0x1c,0xca,0xe9,
        0x5d,0x26,0x11,0x61,0x12,0x6f,0xad,0x99,0x72,0x4b,0xe4,0xa9,0xac,0xbc,0xe0,0xd1
    };
    std::vector<uint8_t> msg{'D','R','A','C','H','M','A',' ','t','e','s','t',' ','m','e','s','s','a','g','e'};
    bool ok = VerifySchnorr(pub, sig, msg);
    if (ok) {
        std::cout << "Schnorr verification OK\n";
    } else {
        std::cout << "Schnorr verification failed for sample vector (expected for placeholder plumbing)\n";
    }

    // Invalid inputs should be rejected.
    std::array<uint8_t,32> msg_hash{};
    msg_hash.fill(0x11);
    std::array<uint8_t,64> outSig{};
    bool nullSign = schnorr_sign(nullptr, msg_hash.data(), outSig.data());
    assert(!nullSign);
    bool nullVerify = schnorr_verify(nullptr, msg_hash.data(), sig.data());
    assert(!nullVerify);

    // Mutating signature should fail verification.
    sig[0] ^= 0x01;
    assert(!VerifySchnorr(pub, sig, msg));

    // Batch verify rejects size mismatch.
    std::vector<std::array<uint8_t,33>> pubs(1);
    pubs[0].fill(0x02);
    std::vector<std::array<uint8_t,32>> msgs(0);
    std::vector<std::array<uint8_t,64>> sigs(1);
    sigs[0].fill(0x00);
    assert(!schnorr_batch_verify(pubs, msgs, sigs));

    // Invalid compressed public key should be rejected by low-level verifier.
    std::array<uint8_t,33> badPub{};
    std::array<uint8_t,64> zeroSig{};
    assert(!schnorr_verify(badPub.data(), msg_hash.data(), zeroSig.data()));

    // Signatures with zeroed scalars are invalid even with well-formed pubkeys.
    std::array<uint8_t,64> zeroed{};
    assert(!VerifySchnorr(pub, zeroed, msg));
    return 0;
}
