#include "../tx/transaction.h"
#include <unordered_map>
#include <fstream>
#include <stdexcept>
#include <mutex>
#include <algorithm>

struct OutPointHash {
    std::size_t operator()(const OutPoint& o) const noexcept {
        size_t h = 0;
        for (auto b : o.hash) h = (h * 131) ^ b;
        h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2));
        return h;
    }
};

struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept {
        return a.index == b.index && std::equal(a.hash.begin(), a.hash.end(), b.hash.begin());
    }
};

class Chainstate {
public:
    explicit Chainstate(const std::string& path): storagePath(path) { Load(); }

    bool HaveUTXO(const OutPoint& out) const {
        std::lock_guard<std::mutex> l(mu);
        return utxos.find(out) != utxos.end();
    }

    TxOut GetUTXO(const OutPoint& out) const {
        std::lock_guard<std::mutex> l(mu);
        auto it = utxos.find(out);
        if (it == utxos.end()) throw std::runtime_error("missing utxo");
        return it->second;
    }

    void AddUTXO(const OutPoint& out, const TxOut& txout) {
        std::lock_guard<std::mutex> l(mu);
        utxos[out] = txout;
    }

    void SpendUTXO(const OutPoint& out) {
        std::lock_guard<std::mutex> l(mu);
        auto it = utxos.find(out);
        if (it == utxos.end()) throw std::runtime_error("spend missing utxo");
        utxos.erase(it);
    }

    void Flush() const { Persist(); }

private:
    std::string storagePath;
    std::unordered_map<OutPoint, TxOut, OutPointHash, OutPointEq> utxos;
    mutable std::mutex mu;

    void Load()
    {
        std::lock_guard<std::mutex> l(mu);
        std::ifstream in(storagePath, std::ios::binary);
        if (!in.good()) return;
        uint32_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        for (uint32_t i = 0; i < count; ++i) {
            OutPoint op{};
            in.read(reinterpret_cast<char*>(op.hash.data()), op.hash.size());
            in.read(reinterpret_cast<char*>(&op.index), sizeof(op.index));
            TxOut txo{};
            in.read(reinterpret_cast<char*>(&txo.value), sizeof(txo.value));
            uint32_t scriptSize = 0;
            in.read(reinterpret_cast<char*>(&scriptSize), sizeof(scriptSize));
            txo.scriptPubKey.resize(scriptSize);
            in.read(reinterpret_cast<char*>(txo.scriptPubKey.data()), scriptSize);
            if (!in) throw std::runtime_error("corrupt utxo set");
            utxos.emplace(op, txo);
        }
    }

    void Persist() const
    {
        std::lock_guard<std::mutex> l(mu);
        std::ofstream out(storagePath, std::ios::binary | std::ios::trunc);
        uint32_t count = static_cast<uint32_t>(utxos.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& entry : utxos) {
            out.write(reinterpret_cast<const char*>(entry.first.hash.data()), entry.first.hash.size());
            out.write(reinterpret_cast<const char*>(&entry.first.index), sizeof(entry.first.index));
            out.write(reinterpret_cast<const char*>(&entry.second.value), sizeof(entry.second.value));
            uint32_t scriptSize = static_cast<uint32_t>(entry.second.scriptPubKey.size());
            out.write(reinterpret_cast<const char*>(&scriptSize), sizeof(scriptSize));
            out.write(reinterpret_cast<const char*>(entry.second.scriptPubKey.data()), scriptSize);
        }
    }
};
