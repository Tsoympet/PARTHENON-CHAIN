#include "../block/block.h"
#include "../pow/difficulty.h"
#include "../merkle/merkle.h"
#include "../consensus/params.h"
#include "../tx/transaction.h"
#include <stdexcept>

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params)
{
    return pow::CheckProofOfWork(BlockHash(header), header.bits, params);
}

bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params)
{
    if (txs.empty()) return false;
    for (const auto& tx : txs) {
        uint64_t totalOut = 0;
        for (const auto& out : tx.vout) {
            totalOut += out.value;
            if (!consensus::MoneyRange(totalOut, params))
                return false;
            if (out.scriptPubKey.size() != 32)
                return false; // enforce schnorr-only pubkeys
        }
    }
    return true;
}

bool ValidateBlock(const Block& block, const consensus::Params& params)
{
    if (!ValidateBlockHeader(block.header, params))
        return false;
    if (!ValidateTransactions(block.transactions, params))
        return false;
    if (ComputeMerkleRoot(block.transactions) != block.header.merkleRoot)
        return false;
    return true;
}
