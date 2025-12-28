#include "params.h"
#include "../block/block.h"
#include "../merkle/merkle.h"
#include "../crypto/tagged_hash.h"
#include <algorithm>
#include <array>
#include <boost/multiprecision/cpp_int.hpp>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

using boost::multiprecision::cpp_int;

namespace {
constexpr uint64_t COIN = 100000000ULL;

[[maybe_unused]] uint32_t CompactFromTarget(const std::array<uint8_t, 32>& target)
{
    // Utility primarily for diagnostics; not used in mining path.
    int32_t size = 32;
    while (size > 0 && target[32 - size] == 0)
        --size;

    uint32_t compact = 0;
    if (size > 3) {
        uint32_t mantissa = (target[32 - size] << 16) | (target[32 - size + 1] << 8) | (target[32 - size + 2]);
        compact = (size << 24) | mantissa;
    } else {
        uint32_t mantissa = 0;
        for (int i = 0; i < size; ++i)
            mantissa |= target[31 - i] << (8 * i);
        compact = (size << 24) | mantissa;
    }
    return compact;
}

std::array<uint8_t, 32> TargetFromCompact(uint32_t compact)
{
    uint32_t exponent = compact >> 24;
    uint32_t mantissa = compact & 0x007fffff;
    bool negative = compact & 0x00800000;
    (void)negative; // Governance-free chain: negative targets are invalid but ignored here.

    cpp_int target = cpp_int(mantissa) << (8 * (static_cast<int>(exponent) - 3));

    std::array<uint8_t, 32> out{};
    for (size_t i = 0; i < out.size(); ++i) {
        out[31 - i] = static_cast<uint8_t>(target & 0xff);
        target >>= 8;
    }
    return out;
}

cpp_int ToInteger(const std::array<uint8_t, 32>& target)
{
    cpp_int result = 0;
    for (uint8_t byte : target)
        result = (result << 8) | byte;
    return result;
}

uint256 ComputeBlockHash(const BlockHeader& header)
{
    return tagged_hash("BLOCK", std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&header), sizeof(BlockHeader)));
}

bool CheckProofOfWork(const BlockHeader& header)
{
    auto targetBytes = TargetFromCompact(header.bits);
    cpp_int target = ToInteger(targetBytes);

    // A target of zero is not permitted.
    if (target == 0)
        return false;

    cpp_int hashVal = ToInteger(ComputeBlockHash(header));
    return hashVal <= target;
}

void MineGenesis(Block& genesis)
{
    // Mine by iterating nonce until proof-of-work satisfies the target encoded in bits.
    for (uint32_t nonce = 0; nonce < std::numeric_limits<uint32_t>::max(); ++nonce) {
        genesis.header.nonce = nonce;
        if (CheckProofOfWork(genesis.header))
            return;
    }
    throw std::runtime_error("Unable to find valid genesis nonce");
}

std::string BuildGenesisScript(const std::string& message)
{
    // Unspendable coinbase: OP_RETURN <message>
    return "OP_RETURN " + message;
}

} // namespace

Block CreateGenesisBlock(const consensus::Params& params)
{
    Transaction coinbase;
    const std::string script = BuildGenesisScript(params.genesisMessage);
    coinbase.vout.push_back(TxOut{50 * COIN, std::vector<uint8_t>(script.begin(), script.end())});

    Block genesis;
    genesis.header.version = 1;
    genesis.header.prevBlockHash = uint256{};
    genesis.header.time = params.nGenesisTime;
    genesis.header.bits = params.nGenesisBits;
    genesis.header.nonce = params.nGenesisNonce;
    genesis.transactions.push_back(coinbase);
    genesis.header.merkleRoot = ComputeMerkleRoot(genesis.transactions);

    // Ensure genesis meets declared proof-of-work; mine if nonce not provided.
    if (!CheckProofOfWork(genesis.header)) {
        // Preserve the supplied time/bits and mine only if the nonce is unset.
        if (params.nGenesisNonce != 0)
            throw std::runtime_error("Provided genesis nonce does not satisfy proof-of-work");
        MineGenesis(genesis);
    }

    return genesis;
}
