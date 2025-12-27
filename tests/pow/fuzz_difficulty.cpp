#include "../../layer1-core/pow/difficulty.h"
#include "../../layer1-core/consensus/params.h"
#include <cstdint>
#include <vector>
#include <fstream>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 8) return 0;
    const auto& params = consensus::Main();
    for (size_t i = 0; i + 8 <= size; i += 8) {
        uint32_t bits = static_cast<uint32_t>(data[i]) << 24 | static_cast<uint32_t>(data[i + 1]) << 16 |
                        static_cast<uint32_t>(data[i + 2]) << 8 | static_cast<uint32_t>(data[i + 3]);
        int64_t timespan = static_cast<int64_t>(data[i + 4]) * params.nPowTargetSpacing;
        try {
            (void)powalgo::CalculateNextWorkRequired(bits, timespan, params);
        } catch (const std::exception&) {
            // Invalid compact encodings or zero timespans are fine to ignore.
        }
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

