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

TxOut MakeTxOut(uint64_t value)
{
    TxOut out{};
    out.value = value;
    out.scriptPubKey.resize(32, 0x01);
    return out;
}

Transaction MakeCoinbase(uint64_t value)
{
    Transaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout = MakeOutPoint(0x00, std::numeric_limits<uint32_t>::max());
    coinbase.vin[0].scriptSig = {0x01, 0x02};
    coinbase.vin[0].sequence = 0xffffffff;
    coinbase.vout.push_back(MakeTxOut(value));
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
        txs.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params)));
        assert(ValidateTransactions(txs, params, 1));
    }

    // Coinbase cannot overpay subsidy when no fees exist.
    {
        std::vector<Transaction> txs;
        txs.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params) + 1));
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
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(2, params));
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
        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(3, params));
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

    return 0;
}
