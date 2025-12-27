#include "validation.h"
#include "../pow/difficulty.h"
#include "../merkle/merkle.h"
#include "../script/interpreter.h"
#include <algorithm>
#include <limits>

namespace {

bool IsNullOutPoint(const OutPoint& prevout)
{
    return std::all_of(prevout.hash.begin(), prevout.hash.end(), [](uint8_t b) { return b == 0; }) &&
           prevout.index == std::numeric_limits<uint32_t>::max();
}

bool IsCoinbase(const Transaction& tx)
{
    return tx.vin.size() == 1 && IsNullOutPoint(tx.vin.front().prevout);
}

} // namespace

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params)
{
    return powalgo::CheckProofOfWork(BlockHash(header), header.bits, params);
}

bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup)
{
    if (txs.empty()) return false;

    // Coinbase must be first and unique
    if (!IsCoinbase(txs.front()))
        return false;

    // Enforce reasonable scriptSig length on coinbase (2-100 bytes)
    const auto& coinbaseSig = txs.front().vin.front().scriptSig;
    if (coinbaseSig.size() < 2 || coinbaseSig.size() > 100)
        return false;

    for (size_t i = 0; i < txs.size(); ++i) {
        const auto& tx = txs[i];

        uint64_t totalOut = 0;
        for (const auto& out : tx.vout) {
            totalOut += out.value;
            if (!consensus::MoneyRange(totalOut, params))
                return false;
            if (out.scriptPubKey.size() != 32)
                return false; // enforce schnorr-only pubkeys
        }

        if (i == 0) {
            // Coinbase may not exceed subsidy (fees not yet tracked in this skeleton)
            uint64_t subsidy = consensus::GetBlockSubsidy(height, params);
            if (totalOut > subsidy)
                return false;
            continue;
        }

        if (IsCoinbase(tx))
            return false; // only the first tx may be coinbase

        if (!lookup)
            return false; // cannot validate spends without a UTXO provider

        uint64_t totalIn = 0;
        for (size_t inIdx = 0; inIdx < tx.vin.size(); ++inIdx) {
            const auto& in = tx.vin[inIdx];
            if (IsNullOutPoint(in.prevout))
                return false;
            if (in.scriptSig.empty())
                return false;

            auto utxo = lookup(in.prevout);
            if (!utxo)
                return false;

            if (!VerifyScript(tx, inIdx, *utxo))
                return false;

            totalIn += utxo->value;
            if (!consensus::MoneyRange(totalIn, params))
                return false;
        }

        if (totalOut > totalIn)
            return false; // overspends
    }

    return true;
}

bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup)
{
    if (!ValidateBlockHeader(block.header, params))
        return false;
    if (!ValidateTransactions(block.transactions, params, height, lookup))
        return false;
    if (ComputeMerkleRoot(block.transactions) != block.header.merkleRoot)
        return false;
    return true;
}
