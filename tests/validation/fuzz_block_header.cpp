#include "../../layer1-core/validation/validation.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/pow/difficulty.h"
#include <cstdint>
#include <cstring>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < sizeof(BlockHeader)) return 0;

    BlockHeader header{};
    std::memcpy(&header, data, sizeof(BlockHeader));

    auto params = consensus::Main();
    BlockValidationOptions opts{};
    opts.medianTimePast = header.time > 0 ? header.time - 1 : 1;
    opts.now = header.time + 1;

    (void)powalgo::CheckProofOfWork(BlockHash(header), header.bits, params);
    (void)ValidateBlockHeader(header, params, opts);
    return 0;
}
