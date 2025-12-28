#pragma once

#include "../tx/transaction.h"
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <vector>

#ifdef DRACHMA_HAVE_LEVELDB
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#endif

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

    // Simple transactional API used by block validation to stage updates before
    // finalizing a new tip. Rollback restores the in-memory view without
    // touching persistent storage.
    void BeginTransaction();
    void Commit();
    void Rollback();

    std::size_t CachedEntries() const;

private:
    std::string storagePath;
    mutable std::unordered_map<OutPoint, TxOut, OutPointHash, OutPointEq> utxos;
    mutable std::unordered_map<OutPoint, TxOut, OutPointHash, OutPointEq> cache;
    std::size_t maxCacheEntries;
    mutable std::mutex mu;
    bool inTransaction{false};
    struct ChangeLog {
        OutPoint out;
        bool hadOld{false};
        TxOut oldValue{};
        bool hadNew{false};
        TxOut newValue{};
    };
    std::vector<ChangeLog> pending;

#ifdef DRACHMA_HAVE_LEVELDB
    std::unique_ptr<leveldb::DB> db;
    bool useDb{false};
#endif

    void Load();
    void Persist() const;
    void MaybeEvict() const;
#ifdef DRACHMA_HAVE_LEVELDB
    void PersistBatch(leveldb::WriteBatch& batch) const;
#endif
};

