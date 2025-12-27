#include "../../layer1-core/crypto/schnorr.h"
#include <cstdint>
#include <vector>
#include <fstream>
#include <array>
#include <algorithm>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 96) // need at least pubkey (32) + sig (64)
        return 0;

    std::array<uint8_t, 32> pubkey{};
    std::copy_n(data, pubkey.size(), pubkey.begin());

    std::array<uint8_t, 64> sig{};
    std::copy_n(data + pubkey.size(), sig.size(), sig.begin());

    std::vector<uint8_t> msg(data + pubkey.size() + sig.size(), data + size);
    (void)VerifySchnorr(pubkey, sig, msg);
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

