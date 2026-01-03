#include <gtest/gtest.h>

#include "../../layer1-core/crypto/schnorr.h"
#include "../../layer1-core/crypto/tagged_hash.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        out.push_back(byte);
    }
    return out;
}

std::array<uint8_t, 32> to_array32(const std::string& hex)
{
    auto bytes = hex_to_bytes(hex);
    if (bytes.size() != 32) {
        throw std::runtime_error("expected 32-byte hex");
    }
    std::array<uint8_t, 32> out{};
    std::copy(bytes.begin(), bytes.end(), out.begin());
    return out;
}

std::array<uint8_t, 64> to_array64(const std::string& hex)
{
    auto bytes = hex_to_bytes(hex);
    if (bytes.size() != 64) {
        throw std::runtime_error("expected 64-byte hex");
    }
    std::array<uint8_t, 64> out{};
    std::copy(bytes.begin(), bytes.end(), out.begin());
    return out;
}

// Helper to wrap a 32-byte x-only pubkey into a 33-byte compressed form using the even-Y prefix expected by BIP-340 vectors.
std::array<uint8_t, 33> to_compressed(const std::array<uint8_t, 32>& xonly)
{
    std::array<uint8_t, 33> out{};
    out[0] = 0x02;
    std::copy(xonly.begin(), xonly.end(), out.begin() + 1);
    return out;
}

} // namespace

TEST(SchnorrBip340, Vector0MatchesReference)
{
    const auto seckey = to_array32("0000000000000000000000000000000000000000000000000000000000000003");
    const auto pubkey = to_array32("F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9");
    const auto msg = to_array32("0000000000000000000000000000000000000000000000000000000000000000");
    const auto aux = to_array32("0000000000000000000000000000000000000000000000000000000000000000");
    const auto expected_sig = to_array64("E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0");

    std::array<uint8_t, 64> sig{};
    ASSERT_TRUE(schnorr_sign_with_aux(seckey.data(), msg.data(), aux.data(), sig.data()));
    EXPECT_EQ(sig, expected_sig);
    EXPECT_TRUE(schnorr_verify(pubkey.data(), msg.data(), sig.data()));

    auto tampered_sig = sig;
    tampered_sig[0] ^= 0x01;
    EXPECT_FALSE(schnorr_verify(pubkey.data(), msg.data(), tampered_sig.data()));
}

TEST(SchnorrBip340, Vector1MatchesReference)
{
    const auto seckey = to_array32("B7E151628AED2A6ABF7158809CF4F3C762E7160F38B4DA56A784D9045190CFEF");
    const auto pubkey = to_array32("DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659");
    const auto msg = to_array32("243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89");
    const auto aux = to_array32("0000000000000000000000000000000000000000000000000000000000000001");
    const auto expected_sig = to_array64("6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A");

    std::array<uint8_t, 64> sig{};
    ASSERT_TRUE(schnorr_sign_with_aux(seckey.data(), msg.data(), aux.data(), sig.data()));
    EXPECT_EQ(sig, expected_sig);
    EXPECT_TRUE(schnorr_verify(pubkey.data(), msg.data(), sig.data()));
}

TEST(SchnorrBip340, NonceReuseIsRejected)
{
    const auto seckey = to_array32("C90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B14E5C9");
    const auto pubkey = to_array32("DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8");
    const auto aux = to_array32("C87AA53824B4D7AE2EB035A2B5BBBCCC080E76CDC6D1692C4B0B62D798E6D906");
    const auto msg1 = to_array32("7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C");
    const auto msg2 = to_array32("5831AAEED7B44BB74E5EAB94BA9D4294C49BCF2A60728D8B4C200F50DD313C1B");

    std::array<uint8_t, 64> sig1{};
    std::array<uint8_t, 64> sig2{};
    ASSERT_TRUE(schnorr_sign_with_aux(seckey.data(), msg1.data(), aux.data(), sig1.data()));
    ASSERT_TRUE(schnorr_sign_with_aux(seckey.data(), msg2.data(), aux.data(), sig2.data()));
    EXPECT_NE(sig1, sig2);
    EXPECT_TRUE(schnorr_verify(pubkey.data(), msg1.data(), sig1.data()));
    EXPECT_FALSE(schnorr_verify(pubkey.data(), msg2.data(), sig1.data()));
}

TEST(SchnorrBip340, InvalidSecretsFail)
{
    std::array<uint8_t, 32> zero{};
    std::array<uint8_t, 32> msg{};
    std::array<uint8_t, 64> sig{};
    EXPECT_FALSE(schnorr_sign_with_aux(zero.data(), msg.data(), nullptr, sig.data()));
}

TEST(SchnorrBip340, RejectsHighSAndMalformedPubkeys)
{
    const auto seckey = to_array32("B7E151628AED2A6ABF7158809CF4F3C762E7160F38B4DA56A784D9045190CFEF");
    const auto pubkey_x = to_array32("DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659");
    const auto msg = to_array32("243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89");
    const auto aux = to_array32("0000000000000000000000000000000000000000000000000000000000000001");
    const auto expected_sig = to_array64("6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A");

    std::array<uint8_t, 64> sig{};
    ASSERT_TRUE(schnorr_sign_with_aux(seckey.data(), msg.data(), aux.data(), sig.data()));
    EXPECT_EQ(sig, expected_sig);

    auto compressed = to_compressed(pubkey_x);
    auto malformed_pub = compressed;
    malformed_pub[0] = 0x05; // invalid prefix -> fails decompression/on-curve check
    EXPECT_FALSE(schnorr_verify(malformed_pub.data(), msg.data(), sig.data()));

    // secp256k1 group order (n); replacing the S half with n ensures schnorr_verify rejects high-S scalars per BIP-340
    constexpr auto kOrder = "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";
    const auto order = to_array32(kOrder);
    auto highS = sig;
    std::copy(order.begin(), order.end(), highS.begin() + 32);
    EXPECT_FALSE(schnorr_verify(compressed.data(), msg.data(), highS.data()));
}

TEST(SchnorrBip340, BatchVerifyDetectsMismatchedMessages)
{
    const auto seckey0 = to_array32("0000000000000000000000000000000000000000000000000000000000000003");
    const auto pubkey0 = to_array32("F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9");
    const auto msg0 = to_array32("0000000000000000000000000000000000000000000000000000000000000000");
    const auto aux0 = to_array32("0000000000000000000000000000000000000000000000000000000000000000");

    const auto seckey1 = to_array32("C90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B14E5C9");
    const auto pubkey1 = to_array32("DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8");
    const auto msg1 = to_array32("7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C");
    const auto aux1 = to_array32("C87AA53824B4D7AE2EB035A2B5BBBCCC080E76CDC6D1692C4B0B62D798E6D906");

    std::array<uint8_t, 64> sig0{};
    std::array<uint8_t, 64> sig1{};
    ASSERT_TRUE(schnorr_sign_with_aux(seckey0.data(), msg0.data(), aux0.data(), sig0.data()));
    ASSERT_TRUE(schnorr_sign_with_aux(seckey1.data(), msg1.data(), aux1.data(), sig1.data()));

    std::vector<std::array<uint8_t, 33>> pubs{to_compressed(pubkey0), to_compressed(pubkey1)};
    std::vector<std::array<uint8_t, 32>> msgs{msg0, msg1};
    std::vector<std::array<uint8_t, 64>> sigs{sig0, sig1};

    ASSERT_TRUE(schnorr_batch_verify(pubs, msgs, sigs));
    msgs[1][0] ^= 0x01; // perturb digest to desync batch equation
    EXPECT_FALSE(schnorr_batch_verify(pubs, msgs, sigs));
}

TEST(TaggedHash, DistinguishesTagsAndInputs)
{
    std::array<uint8_t, 4> small{{0xAA, 0xBB, 0xCC, 0xDD}};
    auto taggedA = tagged_hash("demo/tag", small.data(), small.size());
    auto taggedB = tagged_hash("demo/other", small.data(), small.size());
    EXPECT_NE(taggedA, taggedB);

    auto repeat = tagged_hash("demo/tag", small.data(), small.size());
    EXPECT_EQ(taggedA, repeat); // cache hit

    auto emptyData = tagged_hash("demo/tag", nullptr, 0);
    EXPECT_NE(emptyData, taggedA);
}
