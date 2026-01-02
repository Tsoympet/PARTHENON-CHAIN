#include <gtest/gtest.h>
#include "../../layer2-services/wallet/wallet.h"

using wallet::KeyId;
using wallet::KeyStore;
using wallet::PrivKey;
using wallet::WalletBackend;
using ::TxOut;
using ::OutPoint;

static PrivKey MakeKey(uint8_t seed)
{
    PrivKey key{};
    for (size_t i = 0; i < key.size(); ++i) key[i] = static_cast<uint8_t>(seed + i);
    return key;
}

TEST(Wallet, SignsEachInputUniquely)
{
    KeyStore store;
    WalletBackend backend(store);

    auto priv = MakeKey(1);
    KeyId id = backend.ImportKey(priv);

    OutPoint op1{};
    OutPoint op2{};
    op2.index = 1;

    TxOut utxoA{50'000'000, std::vector<uint8_t>(32, 0xAA)};
    TxOut utxoB{50'000'000, std::vector<uint8_t>(32, 0xBB)};

    backend.AddUTXO(op1, utxoA);
    backend.AddUTXO(op2, utxoB);

    std::vector<TxOut> outputs{TxOut{90'000'000, std::vector<uint8_t>(32, 0xCC)}};
    auto tx = backend.CreateSpend(outputs, id, 1'000'000);

    ASSERT_EQ(tx.vin.size(), 2u);
    EXPECT_EQ(tx.vin[0].scriptSig.size(), tx.vin[1].scriptSig.size());
    EXPECT_EQ(tx.vin[0].scriptSig.size(), 64u);
    EXPECT_NE(tx.vin[0].scriptSig, tx.vin[1].scriptSig);
    ASSERT_EQ(tx.vout.size(), 2u);
    EXPECT_EQ(tx.vout[1].scriptPubKey.size(), 32u);
}

TEST(Wallet, DeterministicSignaturesForSameInput)
{
    KeyStore store;
    WalletBackend backend(store);

    auto priv = MakeKey(9);
    KeyId id = backend.ImportKey(priv);

    OutPoint op{};
    TxOut utxo{10'000'000, std::vector<uint8_t>(32, 0x01)};
    backend.AddUTXO(op, utxo);

    std::vector<TxOut> outputs{TxOut{9'000'000, std::vector<uint8_t>(32, 0x02)}};
    auto txA = backend.CreateSpend(outputs, id, 500'000);
    backend.AddUTXO(op, utxo); // re-seed the wallet for a second identical spend
    auto txB = backend.CreateSpend(outputs, id, 500'000);

    ASSERT_EQ(txA.vin.size(), 1u);
    ASSERT_EQ(txB.vin.size(), 1u);
    EXPECT_EQ(txA.vin[0].scriptSig, txB.vin[0].scriptSig);
    EXPECT_EQ(txA.GetHash(), txB.GetHash());
}
