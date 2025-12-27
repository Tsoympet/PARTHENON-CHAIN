#pragma once

#include "../tx/transaction.h"
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <algorithm>

struct OutPointHash {
    std::size_t operator()(const OutPoint& o) const noexcept;
};
struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept;
};

// Persistent chainstate with a bounded UTXO cache for fast lookups during
// block/transaction validation.
class Chainstate {
public:
    explicit Chainstate(const std::string& path, std::size_t cacheCapacity = 64 * 1024);

    bool HaveUTXO(const OutPoint& out) const;
    std::optional<TxOut> TryGetUTXO(const OutPoint& out) const;
    TxOut GetUTXO(const OutPoint& out) const;

    void AddUTXO(const OutPoint& out, const TxOut& txout);
    void SpendUTXO(const OutPoint& out);
    void Flush() const;

    std::size_t CachedEntries() const;

private:
    std::string storagePath;
    mutable std::unordered_map<OutPoint, TxOut, OutPointHash, OutPointEq> utxos;
    mutable std::unordered_map<OutPoint, TxOut, OutPointHash, OutPointEq> cache;
    std::size_t maxCacheEntries;
    mutable std::mutex mu;

    void Load();
    void Persist() const;
    void MaybeEvict() const;
};

