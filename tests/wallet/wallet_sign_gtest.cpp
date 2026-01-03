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
    TxOut utxo{20'000'000, std::vector<uint8_t>(32, 0x01)};
    utxo.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    backend.AddUTXO(op, utxo);

    std::vector<TxOut> outputs1{TxOut{1'000'000, std::vector<uint8_t>(32, 0xBB)}};
    outputs1[0].assetId = utxo.assetId;
    auto firstSpend = backend.CreateSpend(outputs1, id, 100'000);
    ASSERT_FALSE(firstSpend.vout.empty());
    TxOut confirmed = firstSpend.vout.back(); // change output carries the wallet-derived pubkey
    confirmed.assetId = utxo.assetId;

    OutPoint op2{};
    op2.hash.fill(0x02);
    backend.AddUTXO(op2, confirmed);

    std::vector<TxOut> outputs2{TxOut{500'000, std::vector<uint8_t>(32, 0xCC)}};
    outputs2[0].assetId = utxo.assetId;
    auto tx = backend.CreateSpend(outputs2, id, 50'000);
    ASSERT_EQ(tx.vin.size(), 1u);
    EXPECT_TRUE(VerifyScript(tx, 0, confirmed));

    auto tampered = tx;
    tampered.vin[0].scriptSig.pop_back();
    EXPECT_FALSE(VerifyScript(tampered, 0, confirmed));
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

TEST(Wallet, InsufficientFundsAndAssetIsolation)
{
    KeyStore store;
    WalletBackend backend(store);
    auto priv = MakeKey(12);
    auto id = backend.ImportKey(priv);

    OutPoint op{};
    TxOut utxo{1'000, std::vector<uint8_t>(32, 0x01)};
    utxo.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    backend.AddUTXO(op, utxo);

    // Spending more than available, including fee, throws.
    TxOut outA{900, std::vector<uint8_t>(32, 0x02)};
    outA.assetId = utxo.assetId;
    std::vector<TxOut> outputs{outA};
    EXPECT_THROW(backend.CreateSpend(outputs, id, 200), std::runtime_error);

    // Requesting TLN spend with only DRM balance is rejected during coin selection.
    TxOut tlnOut{100, std::vector<uint8_t>(32, 0x03)};
    tlnOut.assetId = static_cast<uint8_t>(AssetId::TALANTON);
    std::vector<TxOut> tlnOutputs{tlnOut};
    EXPECT_THROW(backend.CreateSpend(tlnOutputs, id, 10), std::runtime_error);
}

TEST(Wallet, HdSeedRequiredBeforeDerive)
{
    KeyStore store;
    WalletBackend backend(store);
    EXPECT_THROW(backend.DeriveChild(wallet::HDNode{}, 0, false), std::runtime_error);

    std::vector<uint8_t> seed(32, 0x01);
    backend.SetHDSeed(seed);
    EXPECT_NO_THROW(backend.DeriveBip44(0, 0, 0));
}

TEST(Wallet, MultisigSpendFailsWithoutInputs)
{
    KeyStore store;
    WalletBackend backend(store);
    auto priv = MakeKey(13);
    backend.ImportKey(priv);

    constexpr uint8_t kScriptByte = 0xAA;
    std::vector<TxOut> outputs{TxOut{1'000, std::vector<uint8_t>(32, kScriptByte)}};
    outputs[0].assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    std::vector<OutPoint> coins{OutPoint{}};
    std::vector<PrivKey> keys{priv};
    EXPECT_THROW(backend.CreateMultisigSpend(outputs, coins, keys, 1, 10), std::runtime_error);
    EXPECT_EQ(backend.GetBalance(), 0u);
}

TEST(Wallet, MultisigRejectsMixedScriptsAndPreservesBalanceOnError)
{
    KeyStore store;
    WalletBackend backend(store);
    auto keyA = MakeKey(21);
    auto keyB = MakeKey(22);
    backend.ImportKey(keyA);
    backend.ImportKey(keyB);

    OutPoint op1{};
    OutPoint op2{};
    op2.index = 1;

    TxOut utxoA{1'000, std::vector<uint8_t>(32, 0xAA)};
    utxoA.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
    TxOut utxoB{1'000, std::vector<uint8_t>(32, 0xBB)};
    utxoB.assetId = utxoA.assetId;
    backend.AddUTXO(op1, utxoA);
    backend.AddUTXO(op2, utxoB);

    backend.SetUTXOLookup([&](const OutPoint& op) -> std::optional<TxOut> {
        if (op.hash == op1.hash && op.index == op1.index) return utxoA;
        if (op.hash == op2.hash && op.index == op2.index) return utxoB;
        return std::nullopt;
    });

    std::vector<TxOut> outputs{TxOut{1'500, std::vector<uint8_t>(32, 0xCC)}};
    outputs[0].assetId = utxoA.assetId;
    std::vector<OutPoint> coins{op1, op2};
    std::vector<PrivKey> keys{keyA, keyB};

    EXPECT_THROW(backend.CreateMultisigSpend(outputs, coins, keys, 2, 100), std::runtime_error);
    EXPECT_EQ(backend.GetBalance(), utxoA.value + utxoB.value);
}

TEST(Keystore, EncryptsAndRejectsBadPassphrase)
{
    wallet::KeyStore store;
    wallet::PrivKey priv = MakeKey(11);
    wallet::KeyId id{}; id.fill(0x01);
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

TEST(Keystore, MissingFileThrows)
{
    wallet::KeyStore store;
    auto tmp = std::filesystem::temp_directory_path() / "keystore_missing.dat";
    std::filesystem::remove(tmp);
    EXPECT_THROW(store.LoadFromFile("pass", tmp.string()), std::runtime_error);
}
