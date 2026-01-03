#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "../runtime/types.h"

namespace sidechain::wasm {

struct ExecutionIdentity {
    ExecutionDomain domain;
    uint8_t asset_id;
};

struct SidechainBlockHeader {
    uint64_t height{0};
    std::array<uint8_t, 32> state_root{};
    std::array<uint8_t, 32> execution_root{};
    std::array<uint8_t, 32> nft_state_root{};
    std::array<uint8_t, 32> market_state_root{};
    std::array<uint8_t, 32> event_root{};
    std::array<uint8_t, 32> main_chain_checkpoint{};
};

bool ValidateAssetDomain(const ExecutionIdentity& identity, std::string& error);
bool ValidateCheckpoint(const SidechainBlockHeader& header,
                        const std::array<uint8_t, 32>& expected_checkpoint,
                        std::string& error);

}  // namespace sidechain::wasm
