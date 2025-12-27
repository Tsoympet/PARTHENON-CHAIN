#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../../layer1-core/block/block.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/pow/difficulty.h"

namespace {

uint256 FromHex(const std::string& hex)
{
    if (hex.size() != 64)
        throw std::invalid_argument("expected 32-byte hex string");
    uint256 out{};
    for (size_t i = 0; i < 32; ++i) {
        auto byte = std::stoul(hex.substr(i * 2, 2), nullptr, 16);
        out[i] = static_cast<uint8_t>(byte);
    }
    return out;
}

void PrintHash(const uint256& h)
{
    for (auto b : h)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <version> <prevhash> <merkleroot> <time> <bits>\n";
        return 1;
    }

    BlockHeader header{};
    header.version = static_cast<uint32_t>(std::stoul(argv[1]));
    header.prevBlockHash = FromHex(argv[2]);
    header.merkleRoot = FromHex(argv[3]);
    header.time = static_cast<uint32_t>(std::stoul(argv[4]));
    header.bits = static_cast<uint32_t>(std::stoul(argv[5], nullptr, 16));
    header.nonce = 0;

    const auto& params = consensus::Main();
    const auto start = std::chrono::steady_clock::now();

    while (true) {
        auto hash = BlockHash(header);
        if (pow::CheckProofOfWork(hash, header.bits, params)) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
            std::cout << "Found nonce: " << header.nonce << " after " << elapsed << "s\n";
            std::cout << "Hash: 0x";
            PrintHash(hash);
            std::cout << "\n";
            break;
        }
        ++header.nonce;
        if (header.nonce == 0) {
            std::cerr << "Exhausted nonce space" << std::endl;
            return 2;
        }
    }

    return 0;
}
