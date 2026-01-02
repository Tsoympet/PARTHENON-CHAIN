#pragma once

#include "keystore/keystore.h"
#include "../../layer1-core/tx/transaction.h"
#include "../../layer1-core/validation/validation.h"
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace wallet {

struct UTXO {
    OutPoint outpoint;
    TxOut txout;
};

struct HDNode {
    PrivKey priv;
    PubKey pub;
    uint32_t depth{0};
    uint32_t childNumber{0};
    std::array<uint8_t, 32> chainCode{};
    uint32_t parentFingerprint{0};
};

class WalletBackend {
public:
    explicit WalletBackend(KeyStore store);

    KeyId ImportKey(const PrivKey& priv);
    bool GetKey(const KeyId& id, PrivKey& out) const;
    void AddUTXO(const OutPoint& op, const TxOut& txout);
    void SetUTXOLookup(UTXOLookup lookup);
    void SyncFromLayer1(const std::vector<OutPoint>& watchlist);
    uint64_t GetBalance() const;
    uint64_t GetBalance(uint8_t assetId) const;
    std::unordered_map<uint8_t, uint64_t> GetBalances() const;
    Transaction CreateSpend(const std::vector<TxOut>& outputs, const KeyId& from, uint64_t fee);

    // HD + multisig helpers
    void SetHDSeed(const std::vector<uint8_t>& seed);
    HDNode DeriveChild(const HDNode& node, uint32_t index, bool hardened);
    HDNode DeriveBip44(uint32_t account, uint32_t change, uint32_t address_index);
    PubKey GenerateAddress(uint32_t account, uint32_t change, uint32_t address_index);
    bool SchnorrSign(const HDNode& node, const std::array<uint8_t, 32>& msg_hash, std::array<uint8_t, 64>& sig_out) const;
    std::vector<uint8_t> BuildMultisigScript(const std::vector<PubKey>& pubs, uint8_t m) const;
    Transaction CreateMultisigSpend(const std::vector<TxOut>& outputs, const std::vector<OutPoint>& coins, const std::vector<PrivKey>& keys, uint8_t threshold, uint64_t fee);

private:
    std::vector<UTXO> SelectCoins(uint64_t amount, std::optional<uint8_t> assetId = std::nullopt) const;
    std::vector<uint8_t> SignDigest(const PrivKey& key, const Transaction& tx, size_t inputIndex) const;
    PubKey DerivePub(const PrivKey& priv) const;
    void RemoveCoins(const std::vector<OutPoint>& used);

    KeyStore m_store;
    std::vector<UTXO> m_utxos;
    UTXOLookup m_lookup;
    mutable std::mutex m_mutex;
    HDNode m_master{};
    bool m_hasSeed{false};
};

} // namespace wallet
