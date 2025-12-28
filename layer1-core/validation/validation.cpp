#include "validation.h"
#include "../merkle/merkle.h"
#include "../pow/pow.h"

namespace validation {

bool CheckTransaction(const Transaction& tx, std::string& error)
{
    if (tx.vin.empty()) {
        error = "TX has no inputs";
        return false;
    }
    if (tx.vout.empty()) {
        error = "TX has no outputs";
        return false;
    }

    uint64_t outSum = 0;
    for (const auto& out : tx.vout) {
        if (out.value == 0) {
            error = "Zero-value output";
            return false;
        }
        outSum += out.value;
    }

    if (tx.isCoinbase()) {
        if (tx.vin.size() != 1) {
            error = "Coinbase must have exactly one input";
            return false;
        }
    } else {
        for (const auto& in : tx.vin) {
            if (in.prevout.txid.empty()) {
                error = "Non-coinbase TX with empty prevout";
                return false;
            }
        }
    }

    return true;
}

bool CheckBlockStructure(const Block& block, std::string& error)
{
    if (block.vtx.empty()) {
        error = "Block has no transactions";
        return false;
    }

    if (!block.vtx[0].isCoinbase()) {
        error = "First TX is not coinbase";
        return false;
    }

    for (size_t i = 0; i < block.vtx.size(); ++i) {
        std::string txerr;
        if (!CheckTransaction(block.vtx[i], txerr)) {
            error = "Invalid TX at index " + std::to_string(i) + ": " + txerr;
            return false;
        }
    }

    return true;
}

bool CheckMerkleRoot(const Block& block, std::string& error)
{
    std::vector<std::vector<uint8_t>> leaves;
    for (const auto& tx : block.vtx) {
        auto raw = tx.serialize();
        leaves.push_back(raw);
    }

    auto computed = merkle::ComputeMerkleRoot(leaves);
    if (computed != block.header.merkleRoot) {
        error = "Merkle root mismatch";
        return false;
    }

    return true;
}

bool CheckProofOfWork(const BlockHeader& header,
                      const ConsensusParams& params,
                      std::string& error)
{
    if (!pow::CheckProofOfWork(header.GetHash(), header.nBits, params)) {
        error = "Invalid proof of work";
        return false;
    }
    return true;
}

bool ConnectBlock(const Block& block,
                  Chainstate& chainstate,
                  uint32_t height,
                  BlockUndo& undo,
                  std::string& error)
{
    // Structural checks
    if (!CheckBlockStructure(block, error)) {
        return false;
    }

    // Apply TXs to chainstate (UTXO)
    try {
        chainstate.ApplyBlock(block.vtx, height, undo);
    } catch (const std::exception& e) {
        error = e.what();
        return false;
    }

    return true;
}

} // namespace validation
