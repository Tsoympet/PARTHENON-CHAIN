#include <gtest/gtest.h>
#include "../../layer2-services/wallet/wallet.h"
#include "../../layer1-core/script/interpreter.h"
#include <filesystem>

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

TEST(Wallet, ScriptInterpreterAcceptsWalletSigs)
{
    KeyStore store;
    WalletBackend backend(store);

    auto priv = MakeKey(5);
    KeyId id = backend.ImportKey(priv);

    OutPoint op{};
    TxOut utxo{10'000'000, std::vector<uint8_t>(32, 0x01)};
    utxo.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    backend.AddUTXO(op, utxo);

    std::vector<TxOut> outputs{TxOut{9'000'000, std::vector<uint8_t>(32, 0xBB)}};
    outputs[0].assetId = utxo.assetId;
    auto tx = backend.CreateSpend(outputs, id, 500'000);
    ASSERT_EQ(tx.vin.size(), 1u);
    utxo.scriptPubKey = tx.vout.back().scriptPubKey;
    EXPECT_TRUE(VerifyScript(tx, 0, utxo));

    auto tampered = tx;
    tampered.vin[0].scriptSig.pop_back();
    EXPECT_FALSE(VerifyScript(tampered, 0, utxo));
}

TEST(Wallet, ThrowsOnMissingKeyOrFunds)
{
    KeyStore store;
    WalletBackend backend(store);

    TxOut out{1'000, std::vector<uint8_t>(32, 0x01)};
    std::vector<TxOut> outputs{out};

    KeyId fake{};
    EXPECT_THROW(backend.CreateSpend(outputs, fake, 100), std::runtime_error);

    auto priv = MakeKey(3);
    auto id = backend.ImportKey(priv);
    EXPECT_THROW(backend.CreateSpend(outputs, id, 100), std::runtime_error);
}

TEST(Wallet, RejectsMixedAssetOutputs)
{
    KeyStore store;
    WalletBackend backend(store);
    auto priv = MakeKey(7);
    auto id = backend.ImportKey(priv);

    OutPoint op{};
    TxOut utxo{20'000, std::vector<uint8_t>(32, 0x01)};
    utxo.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    backend.AddUTXO(op, utxo);

    TxOut outA{5'000, std::vector<uint8_t>(32, 0x02)};
    outA.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    TxOut outB{5'000, std::vector<uint8_t>(32, 0x03)};
    outB.assetId = static_cast<uint8_t>(AssetId::OBOLOS);
    std::vector<TxOut> outputs{outA, outB};
    EXPECT_THROW(backend.CreateSpend(outputs, id, 500), std::runtime_error);
}

TEST(Keystore, EncryptsAndRejectsBadPassphrase)
{
    wallet::KeyStore store;
    wallet::PrivKey priv = MakeKey(11);
    wallet::KeyId id{}; id.fill(1);
    store.Import(id, priv);

    auto tmp = std::filesystem::temp_directory_path() / "keystore_test.dat";
    store.EncryptToFile("secret", tmp.string());

    wallet::KeyStore reloaded;
    reloaded.LoadFromFile("secret", tmp.string());
    wallet::PrivKey loaded{};
    ASSERT_TRUE(reloaded.Get(id, loaded));
    EXPECT_EQ(loaded, priv);

    wallet::KeyStore badPass;
    EXPECT_THROW(badPass.LoadFromFile("wrong", tmp.string()), std::runtime_error);
    std::filesystem::remove(tmp);
}
