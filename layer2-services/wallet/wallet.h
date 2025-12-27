#pragma once

#include "keystore/keystore.h"
#include "../../layer1-core/tx/transaction.h"
#include <mutex>
#include <unordered_map>

namespace wallet {

struct UTXO {
    OutPoint outpoint;
    TxOut txout;
};

class WalletBackend {
public:
    explicit WalletBackend(KeyStore store);

    KeyId ImportKey(const PrivKey& priv);
    bool GetKey(const KeyId& id, PrivKey& out) const;
    void AddUTXO(const OutPoint& op, const TxOut& txout);
    uint64_t GetBalance() const;
    Transaction CreateSpend(const std::vector<TxOut>& outputs, const KeyId& from, uint64_t fee);

private:
    std::vector<UTXO> SelectCoins(uint64_t amount) const;
    std::vector<uint8_t> DummySignature(const PrivKey& key, const Transaction& tx) const;
    void RemoveCoins(const std::vector<OutPoint>& used);

    KeyStore m_store;
    std::vector<UTXO> m_utxos;
    mutable std::mutex m_mutex;
};

} // namespace wallet
