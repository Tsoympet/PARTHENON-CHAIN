#include "../../layer1-core/chainstate/coins.h"
#include <cassert>
#include <filesystem>
#include <vector>

namespace {

OutPoint MakeOutPoint(uint8_t seed, uint32_t index)
{
    OutPoint op{};
    op.hash.fill(seed);
    op.index = index;
    return op;
}

TxOut MakeOutput(uint64_t value, uint8_t tag, uint8_t asset = 1)
{
    TxOut out{};
    out.value = value;
    out.scriptPubKey.assign(32, tag);
    out.assetId = asset;
    return out;
}

} // namespace

int main()
{
    std::filesystem::path temp = std::filesystem::temp_directory_path() / "drachma_chainstate.dat";
    std::error_code ec;
    std::filesystem::remove(temp, ec);

    // Persist, reload, and enforce spends.
    {
        Chainstate cs(temp.string(), 8);
        auto op = MakeOutPoint(0x01, 0);
        auto out = MakeOutput(50, 0xAA, 2);
        cs.AddUTXO(op, out);
        cs.Flush();
    }
    {
        Chainstate cs(temp.string(), 8);
        auto op = MakeOutPoint(0x01, 0);
        auto loaded = cs.GetUTXO(op);
        assert(loaded.value == 50);
        assert(loaded.assetId == 2);
        bool threw = false;
        cs.SpendUTXO(op);
        try {
            (void)cs.GetUTXO(op);
        } catch (const std::exception&) {
            threw = true;
        }
        assert(threw);
    }

    // Cache should evict when capacity exceeded but keep lookups correct.
    {
        Chainstate cs(temp.string(), 2);
        auto opA = MakeOutPoint(0x02, 0);
        auto opB = MakeOutPoint(0x03, 0);
        auto opC = MakeOutPoint(0x04, 0);
        cs.AddUTXO(opA, MakeOutput(75, 0xAB));
        cs.AddUTXO(opB, MakeOutput(80, 0xAC));
        cs.AddUTXO(opC, MakeOutput(90, 0xAD));

        (void)cs.TryGetUTXO(opA);
        (void)cs.TryGetUTXO(opB);
        (void)cs.TryGetUTXO(opC);
        assert(cs.CachedEntries() <= 2);
        assert(cs.GetUTXO(opB).value == 80);
        assert(cs.GetUTXO(opC).value == 90);
    }

    std::filesystem::remove(temp, ec);
    return 0;
}
