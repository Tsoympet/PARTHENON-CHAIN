#include "../../layer1-core/validation/validation.h"
#include "../../layer1-core/merkle/merkle.h"
#include "../../layer1-core/consensus/params.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <unordered_map>

namespace {

consensus::Params LooseParams()
{
    consensus::Params p = consensus::Testnet();
    p.nGenesisBits = 0x207fffff; // easiest possible target to smooth tests
    p.fPowAllowMinDifficultyBlocks = true;
    return p;
}

OutPoint NullPrevout()
{
    OutPoint op{};
    op.index = std::numeric_limits<uint32_t>::max();
    op.hash.fill(0);
    return op;
}

// Deterministic hash mixing constants (FNV-style multiplier and golden-ratio offset) kept local to mirror the OutPointHasher used in tests/validation/validation_tests.cpp so cache behavior stays identical.
constexpr size_t kHashMultiplier = 131;
constexpr uint64_t kHashOffset = 0x9e3779b97f4a7c15ULL;

struct OutPointHasher {
    std::size_t operator()(const OutPoint& o) const noexcept
    {
        size_t h = 0;
        for (auto b : o.hash) h = (h * kHashMultiplier) ^ b; // multiplicative mix of hash bytes
        // Fibonacci hashing offset with shift/xor avalanche (borrowed from boost hash_combine) keeps distribution stable
        h ^= static_cast<size_t>(o.index + kHashOffset + (h << 6) + (h >> 2));
        return h;
    }
};

struct OutPointEq {
    bool operator()(const OutPoint& a, const OutPoint& b) const noexcept
    {
        return a.index == b.index && a.hash == b.hash;
    }
};

Transaction MakeCoinbase(uint64_t value)
{
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].prevout = NullPrevout();
    tx.vin[0].scriptSig = {0x01, 0x02};
    tx.vin[0].sequence = 0xffffffff;
    tx.vout[0].value = value;
    tx.vout[0].scriptPubKey.assign(32, 0x01);
    tx.vin[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    tx.vout[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    return tx;
}

TxOut MakeTxOut(uint64_t value, uint8_t asset = static_cast<uint8_t>(AssetId::TALANTON))
{
    TxOut out{};
    out.value = value;
    out.assetId = asset;
    out.scriptPubKey.assign(32, 0x01);
    return out;
}

} // namespace

int main()
{
    const auto params = LooseParams();

    // Block header future-drift and median time past enforcement.
    {
        BlockHeader h{};
        h.bits = params.nGenesisBits;
        h.time = 2000;
        BlockValidationOptions opts;
        opts.medianTimePast = 2100;
        assert(!ValidateBlockHeader(h, params, opts));

        opts.medianTimePast = 1500;
        opts.now = 1500;
        opts.maxFutureDrift = 10;
        h.time = 1520;
        assert(!ValidateBlockHeader(h, params, opts));

        h.time = 1505;
        assert(ValidateBlockHeader(h, params, opts));
    }

    // Fully validate a single-coinbase block with Merkle root and PoW limits.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1600;
        block.header.version = 1;
        block.transactions.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);
        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(ValidateBlock(block, params, 1, {}, opts));
    }

    // Reject when Merkle root mismatches transactions.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1700;
        block.header.version = 1;
        block.transactions.push_back(MakeCoinbase(consensus::GetBlockSubsidy(2, params, static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot.fill(0xFF);
        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(!ValidateBlock(block, params, 2, {}, opts));
    }

    // Enforce nft_state_root anchoring when requested by the caller.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 1800;
        block.header.version = 1;
        const int height = 3;
        block.transactions.push_back(MakeCoinbase(
            consensus::GetBlockSubsidy(height, params,
                                       static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);

        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        opts.requireNftStateRoot = true;

        // Missing anchor should be rejected.
        assert(!ValidateBlock(block, params, height, {}, opts));

        // Mismatched anchor should be rejected.
        opts.nftStateRoot.fill(0x01);
        opts.expectedNftStateRoot.fill(0x02);
        assert(!ValidateBlock(block, params, height, {}, opts));

        // Matching, non-zero anchor should succeed.
        opts.expectedNftStateRoot = opts.nftStateRoot;
        assert(ValidateBlock(block, params, height, {}, opts));

        // Non-zero anchor with unspecified expected root should also succeed.
        opts.expectedNftStateRoot.fill(0);
        assert(ValidateBlock(block, params, height, {}, opts));
    }

    // PoS blocks must use even timestamps and valid staking outputs.
    {
        OutPoint prev{};
        prev.hash.fill(0xAA);
        prev.index = 0;
        TxOut utxo{};
        utxo.value = 1000;
        utxo.assetId = static_cast<uint8_t>(AssetId::DRACHMA);
        utxo.scriptPubKey.assign(32, 0x02);
        auto lookup = [prev, utxo](const OutPoint& op) -> std::optional<TxOut> {
            if (op.hash == prev.hash && op.index == prev.index) return utxo;
            return std::nullopt;
        };

        Block posBlock{};
        posBlock.header.bits = params.nGenesisBits;
        posBlock.header.time = 2001; // odd -> rejected
        posBlock.header.version = 1;

        Transaction stake;
        stake.vin.resize(1);
        stake.vin[0].prevout = prev;
        stake.vin[0].scriptSig = {0x01};
        stake.vin[0].assetId = utxo.assetId;
        stake.vout.resize(2);
        stake.vout[0].assetId = utxo.assetId;
        stake.vout[0].scriptPubKey.assign(32, 0x02);
        stake.vout[1] = stake.vout[0];
        stake.vout[1].value = utxo.value;

        posBlock.transactions = {stake};
        assert(!ValidateBlock(posBlock, params, params.nPoSActivationHeight, lookup));

        posBlock.header.time = 2002; // even, proceeds to transaction checks
        stake.vout[0].value = 1; // invalid: first output must be zero
        posBlock.transactions[0] = stake;
        assert(!ValidateBlock(posBlock, params, params.nPoSActivationHeight, lookup));
    }

    // Blocks without transactions are invalid regardless of header contents.
    {
        Block empty{};
        empty.header.bits = params.nGenesisBits;
        empty.header.time = 2100;
        empty.header.version = 1;
        BlockValidationOptions opts;
        opts.medianTimePast = 2000;
        opts.now = 2100;
        assert(!ValidateBlock(empty, params, 5, {}, opts));
    }

    // Invalid header (zero median time past) should short-circuit before touching state/lookups.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 2150;
        block.header.version = 1;
        block.transactions.push_back(MakeCoinbase(consensus::GetBlockSubsidy(4, params, static_cast<uint8_t>(AssetId::TALANTON))));
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);
        size_t lookups = 0;
        auto counting = [&lookups](const OutPoint&) -> std::optional<TxOut> {
            ++lookups;
            return std::nullopt;
        };
        BlockValidationOptions opts;
        opts.medianTimePast = 0; // forces rejection before transaction checks
        opts.now = block.header.time;
        assert(!ValidateBlock(block, params, 4, counting, opts));
        assert(lookups == 0);
    }

    // Duplicate prevouts inside a block must be rejected during full block validation.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 2160;
        block.header.version = 1;

        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(5, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(25));
        spend.vin.resize(1);
        spend.vin[0].prevout = NullPrevout();
        spend.vin[0].prevout.hash.fill(0x99);
        spend.vin[0].prevout.index = 1;
        spend.vin[0].scriptSig = {0x01, 0x02};
        spend.vin[0].assetId = static_cast<uint8_t>(AssetId::DRACHMA);
        Transaction duplicate = spend;

        block.transactions = {cb, spend, duplicate};
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);

        std::unordered_map<OutPoint, TxOut, OutPointHasher, OutPointEq> utxos;
        utxos[spend.vin[0].prevout] = MakeTxOut(30, spend.vin[0].assetId);
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };

        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(!ValidateBlock(block, params, 5, lookup, opts));
    }

    // Asset-id mismatches between inputs and referenced UTXOs are rejected.
    {
        Block block{};
        block.header.bits = params.nGenesisBits;
        block.header.time = 2170;
        block.header.version = 1;

        Transaction cb = MakeCoinbase(consensus::GetBlockSubsidy(6, params, static_cast<uint8_t>(AssetId::TALANTON)));
        Transaction spend;
        spend.vout.push_back(MakeTxOut(10, static_cast<uint8_t>(AssetId::DRACHMA)));
        spend.vin.resize(1);
        spend.vin[0].prevout = NullPrevout();
        spend.vin[0].prevout.hash.fill(0x42);
        spend.vin[0].prevout.index = 2;
        spend.vin[0].assetId = static_cast<uint8_t>(AssetId::OBOLOS); // mismatched tag
        spend.vin[0].scriptSig = {0x01};
        block.transactions = {cb, spend};
        block.header.merkleRoot = ComputeMerkleRoot(block.transactions);

        std::unordered_map<OutPoint, TxOut, OutPointHasher, OutPointEq> utxos;
        utxos[spend.vin[0].prevout] = MakeTxOut(15, static_cast<uint8_t>(AssetId::DRACHMA));
        auto lookup = [&utxos](const OutPoint& op) -> std::optional<TxOut> {
            auto it = utxos.find(op);
            if (it == utxos.end()) return std::nullopt;
            return it->second;
        };

        BlockValidationOptions opts;
        opts.medianTimePast = block.header.time - 1;
        opts.now = block.header.time;
        assert(!ValidateBlock(block, params, 6, lookup, opts));
    }

    return 0;
}
