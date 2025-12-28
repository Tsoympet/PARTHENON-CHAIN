#include "../../layer1-core/crypto/tagged_hash.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return 0;

    size_t tagLen = std::min<size_t>(size / 4, 32);
    std::string tag(reinterpret_cast<const char*>(data), tagLen);
    const uint8_t* payload = data + tagLen;
    size_t payloadSize = size - tagLen;
    (void)tagged_hash(tag, std::span<const uint8_t>(payload, payloadSize));
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
        return 0;

    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), {});
    LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
    return 0;
}

