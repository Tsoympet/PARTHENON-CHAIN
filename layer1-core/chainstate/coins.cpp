#include "coins.h"
#include <fstream>
#include <stdexcept>
#include <algorithm>

std::size_t OutPointHash::operator()(const OutPoint& o) const noexcept
{
    size_t h = 0;
    for (auto b : o.hash) h = (h * 131) ^ b;
    h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2));
    return h;
}

bool OutPointEq::operator()(const OutPoint& a, const OutPoint& b) const noexcept
{
    return a.index == b.index && std::equal(a.hash.begin(), a.hash.end(), b.hash.begin());
}

Chainstate::Chainstate(const std::string& path, std::size_t cacheCapacity)
    : storagePath(path), maxCacheEntries(cacheCapacity)
{
    Load();
}

bool Chainstate::HaveUTXO(const OutPoint& out) const
{
    std::lock_guard<std::mutex> l(mu);
    if (cache.find(out) != cache.end())
        return true;
    return utxos.find(out) != utxos.end();
}

std::optional<TxOut> Chainstate::TryGetUTXO(const OutPoint& out) const
{
    std::lock_guard<std::mutex> l(mu);
    auto itCache = cache.find(out);
    if (itCache != cache.end())
        return itCache->second;

    auto it = utxos.find(out);
    if (it == utxos.end())
        return std::nullopt;

    cache.emplace(out, it->second);
    MaybeEvict();
    return it->second;
}

TxOut Chainstate::GetUTXO(const OutPoint& out) const
{
    auto coin = TryGetUTXO(out);
    if (!coin)
        throw std::runtime_error("missing utxo");
    return *coin;
}

void Chainstate::AddUTXO(const OutPoint& out, const TxOut& txout)
{
    std::lock_guard<std::mutex> l(mu);
    utxos[out] = txout;
    cache[out] = txout;
    MaybeEvict();
}

void Chainstate::SpendUTXO(const OutPoint& out)
{
    std::lock_guard<std::mutex> l(mu);
    auto it = utxos.find(out);
    if (it == utxos.end()) throw std::runtime_error("spend missing utxo");
    utxos.erase(it);
    cache.erase(out);
}

void Chainstate::Flush() const { Persist(); }

std::size_t Chainstate::CachedEntries() const
{
    std::lock_guard<std::mutex> l(mu);
    return cache.size();
}

void Chainstate::Load()
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

void Chainstate::Persist() const
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

void Chainstate::MaybeEvict() const
{
    if (cache.size() <= maxCacheEntries)
        return;

    // Evict the oldest entries deterministically to prevent unbounded growth
    // and keep hot coins resident.
    const size_t target = maxCacheEntries / 2;
    auto it = cache.begin();
    while (cache.size() > target && it != cache.end()) {
        it = cache.erase(it);
    }
}
