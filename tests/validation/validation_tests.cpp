#include "../../layer1-core/validation/validation.h"
#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <limits>

namespace {

struct OutPointHasher {
    std::size_t operator()(const OutPoint& o) const noexcept
    {
        size_t h = 0;
        for (auto b : o.hash) h = (h * 131) ^ b;
        h ^= static_cast<size_t>(o.index + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
        return h;
    }
};

struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept
    {
        return a.index == b.index && a.hash == b.hash;
    }
};

OutPoint MakeOutPoint(uint8_t seed, uint32_t index)
{
    OutPoint op{};
    op.index = index;
    op.hash.fill(seed);
    return op;
}

TxOut MakeTxOut(uint64_t value, uint8_t asset = static_cast<uint8_t>(AssetId::TALANTON))
{
    TxOut out{};
    out.value = value;
    out.scriptPubKey.resize(32, 0x01);
    out.assetId = asset;
    return out;
}

Transaction MakeCoinbase(uint64_t value)
{
    Transaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout = MakeOutPoint(0x00, std::numeric_limits<uint32_t>::max());
    coinbase.vin[0].scriptSig = {0x01, 0x02};
    coinbase.vin[0].sequence = 0xffffffff;
    TxOut reward = MakeTxOut(value);
    coinbase.vin[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    coinbase.vout.push_back(reward);
    return coinbase;
}

using UTXOSet = std::unordered_map<OutPoint, TxOut, OutPointHasher, OutPointEq>;

} // namespace

int main()
{
    const auto& params = consensus::Main();

    // Valid coinbase-only block at height 1 should pass.
    {
        std::vector<Transaction> txs;
        txs.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON))));
        assert(ValidateTransactions(txs, params, 1));
    }

    // Coinbase cannot overpay subsidy when no fees exist.
    {
        std::vector<Transaction> txs;
        txs.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON)) + 1));
        assert(!ValidateTransactions(txs, params, 1));
    }

    // Reject outputs beyond allowed money range.
    {
        Transaction cb = MakeCoinbase(params.nMaxMoneyOut + 1);
        std::vector<Transaction> txs{cb};
        assert(!ValidateTransactions(txs, params, 1));
    }

    // Reject duplicate prevouts inside the same block before script execution.
    {
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(2, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(50));
        spend.vin.resize(2);
        spend.vin[0].prevout = MakeOutPoint(0xAA, 1);
        spend.vin[0].scriptSig = {0x01};
        spend.vin[1] = spend.vin[0];
        std::vector<Transaction> txs{cb, spend};
        UTXOSet utxos;
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };
        assert(!ValidateTransactions(txs, params, 2, lookup));
    }

    // Reject spends that reference missing UTXOs.
    {
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(3, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(25));
        spend.vin.resize(1);
        spend.vin[0].prevout = MakeOutPoint(0xBB, 2);
        spend.vin[0].scriptSig = {0x01};
        std::vector<Transaction> txs{cb, spend};
        UTXOSet utxos; // empty
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };
        assert(!ValidateTransactions(txs, params, 3, lookup));
    }

    // Reject mixed-asset outputs within a single transaction.
    {
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(4, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(10, static_cast<uint8_t>(AssetId::DRACHMA)));
        spend.vout.push_back(MakeTxOut(5, static_cast<uint8_t>(AssetId::OBOLOS)));
        spend.vin.resize(1);
        spend.vin[0].prevout = MakeOutPoint(0xCC, 0);
        spend.vin[0].scriptSig = {0x01};
        std::vector<Transaction> txs{cb, spend};
        UTXOSet utxos;
        utxos[spend.vin[0].prevout] = MakeTxOut(20, static_cast<uint8_t>(AssetId::DRACHMA));
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };
        assert(!ValidateTransactions(txs, params, 4, lookup));
    }

    // Reject asset-id mismatch between input tag and referenced UTXO/output.
    {
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(5, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(8, static_cast<uint8_t>(AssetId::DRACHMA)));
        spend.vin.resize(1);
        spend.vin[0].prevout = MakeOutPoint(0xDD, 0);
        spend.vin[0].scriptSig = {0x01};
        spend.vin[0].assetId = static_cast<uint8_t>(AssetId::OBOLOS);
        std::vector<Transaction> txs{cb, spend};
        UTXOSet utxos;
        utxos[spend.vin[0].prevout] = MakeTxOut(10, static_cast<uint8_t>(AssetId::DRACHMA));
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };
        assert(!ValidateTransactions(txs, params, 5, lookup));
    }

    // PoW coinbase must not mint PoS-only assets.
    {
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON)));
        cb.vout[0].assetId = static_cast<uint8_t>(AssetId::OBOLOS);
        cb.vin[0].assetId = cb.vout[0].assetId;
        std::vector<Transaction> txs{cb};
        assert(!ValidateTransactions(txs, params, 1));
    }

    // PoS blocks cannot stake PoW-only assets (TLN).
    {
        UTXOSet utxos;
        OutPoint prev = MakeOutPoint(0xEF, 0);
        TxOut stakeOut = MakeTxOut(1000, static_cast<uint8_t>(AssetId::TALANTON));
        utxos[prev] = stakeOut;

        Transaction stake;
        stake.vin.resize(1);
        stake.vin[0].prevout = prev;
        stake.vin[0].scriptSig = {0x01};
        stake.vin[0].sequence = 0xffffffff;
        stake.vin[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
        stake.vout.push_back(MakeTxOut(0, stakeOut.assetId));
        stake.vout.push_back(MakeTxOut(1000, stakeOut.assetId));

        std::vector<Transaction> txs{stake};
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };
        assert(!ValidateTransactions(txs, params, 2, lookup, true, params.nGenesisBits, 100));
    }

    return 0;
}
