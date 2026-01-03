#include "validator.h"
#include "../runtime/types.h"

namespace sidechain::wasm {

namespace {
bool MatchesAsset(ExecutionDomain domain, uint8_t asset_id) {
    switch (domain) {
        case ExecutionDomain::SmartContract:
            return asset_id == kAssetDrm;
        case ExecutionDomain::NFT:
            // NFTs are Layer-2 cultural records and do not bind to TLN or any asset.
            (void)asset_id;
            return true;
        case ExecutionDomain::Dapp:
            return asset_id == kAssetObl;
        default:
            return false;
    }
}
}  // namespace

bool ValidateAssetDomain(const ExecutionIdentity& identity, std::string& error) {
    if (!MatchesAsset(identity.domain, identity.asset_id)) {
        error = "asset/domain violation";
        return false;
    }
    return true;
}

bool ValidateCheckpoint(const SidechainBlockHeader& header,
                        const std::array<uint8_t, 32>& expected_checkpoint,
                        std::string& error) {
    if (header.main_chain_checkpoint != expected_checkpoint) {
        error = "checkpoint mismatch";
        return false;
    }
    if (header.state_root == std::array<uint8_t, 32>{} ||
        header.execution_root == std::array<uint8_t, 32>{} ||
        header.nft_state_root == std::array<uint8_t, 32>{} ||
        header.market_state_root == std::array<uint8_t, 32>{} ||
        header.event_root == std::array<uint8_t, 32>{}) {
        error = "missing execution anchors";
        return false;
    }
    return true;
}

}  // namespace sidechain::wasm
