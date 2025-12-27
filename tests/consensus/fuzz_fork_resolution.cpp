#include "../../layer1-core/consensus/fork_resolution.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/block/block.h"
#include <cstdint>
#include <vector>
#include <fstream>

static BlockHeader MakeHeader(const uint256& prev, uint32_t time, uint32_t bits)
{
    BlockHeader h{};
    h.version = 1;
    h.prevBlockHash = prev;
    h.merkleRoot.fill(0);
    h.time = time;
    h.bits = bits;
    h.nonce = 0;
    return h;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 8) return 0;
    const auto& params = consensus::Main();
    consensus::ForkResolver resolver(/*finalizationDepth=*/4, /*reorgMargin=*/500);

    uint256 tip{};
    uint32_t height = 0;
    for (size_t i = 0; i + 8 <= size; i += 8) {
        uint32_t time = static_cast<uint32_t>(data[i]) << 24 | static_cast<uint32_t>(data[i + 1]) << 16 |
                        static_cast<uint32_t>(data[i + 2]) << 8 | static_cast<uint32_t>(data[i + 3]);
        uint32_t bits = params.nGenesisBits - (data[i + 4] % 4); // stay near pow limit
        BlockHeader h = MakeHeader(tip, time ? time : params.nGenesisTime, bits);
        uint256 hash = BlockHash(h);
        resolver.ConsiderHeader(h, hash, tip, height, params);
        tip = hash;
        ++height;
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) return 0;
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), {});
    LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
    return 0;
}

