#include "wasm_rpc.h"

#include "../wasm/validator/validator.h"
#include <sstream>

namespace sidechain::rpc {

using sidechain::wasm::ExecutionDomain;
using sidechain::wasm::ExecutionRequest;
using sidechain::wasm::ExecutionResult;
using sidechain::wasm::Instruction;
using sidechain::wasm::kAssetDrm;
using sidechain::wasm::kAssetObl;
using sidechain::wasm::kAssetTln;
using sidechain::wasm::kMaxRoyaltyBps;

namespace {
constexpr uint64_t kFixedNftGas = 50;
constexpr char kNftModule[] = "nft";
constexpr char kNftMetaModule[] = "nft:meta";
constexpr char kNftCoreModule[] = "nft:core";
constexpr char kNftEventModule[] = "nft:events";
constexpr char kMarketListingModule[] = "nft:market:listing";
constexpr char kMarketBidModule[] = "nft:market:bids";
constexpr char kMarketBalanceModule[] = "nft:market:balances";

bool Validate(ExecutionDomain domain, uint8_t asset, std::string& error) {
    return sidechain::wasm::ValidateAssetDomain({domain, asset}, error);
}

bool ApplyFixedNftCost(uint64_t provided_limit, ExecutionResult& res) {
    const uint64_t limit = provided_limit == 0 ? kFixedNftGas : provided_limit;
    if (limit < kFixedNftGas) {
        res.error = "out of gas";
        res.gas_used = limit;
        return false;
    }
    res.gas_used = kFixedNftGas;
    return true;
}

struct NftRecord {
    std::string owner;
    std::string creator;
    std::string metadata_hash;
    std::string canon_reference_hash;
    uint64_t mint_height{0};
    uint16_t royalty_bps{0};
};

std::vector<uint8_t> Serialize(const NftRecord& rec) {
    std::stringstream ss;
    ss << rec.owner << '|' << rec.creator << '|' << rec.metadata_hash << '|'
       << rec.canon_reference_hash << '|' << rec.mint_height << '|' << rec.royalty_bps;
    const auto s = ss.str();
    return std::vector<uint8_t>(s.begin(), s.end());
}

bool Deserialize(const std::vector<uint8_t>& data, NftRecord& out) {
    const std::string s(data.begin(), data.end());
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }
    if (parts.size() != 6) {
        return false;
    }
    out.owner = parts[0];
    out.creator = parts[1];
    out.metadata_hash = parts[2];
    out.canon_reference_hash = parts[3];
    try {
        out.mint_height = std::stoull(parts[4]);
        out.royalty_bps = static_cast<uint16_t>(std::stoul(parts[5]));
    } catch (...) {
        return false;
    }
    return true;
}

struct Listing {
    std::string seller;
    uint8_t asset{0};
    uint64_t price{0};
    uint64_t height{0};
};

struct Bid {
    std::string bidder;
    uint8_t asset{0};
    uint64_t price{0};
    uint64_t height{0};
};

template <typename T>
std::vector<uint8_t> SerializePriceEntry(const T& entry, const std::string& id) {
    std::stringstream ss;
    ss << id << '|' << static_cast<uint32_t>(entry.asset) << '|' << entry.price << '|'
       << entry.height;
    const auto s = ss.str();
    return std::vector<uint8_t>(s.begin(), s.end());
}

bool DeserializeListing(const std::vector<uint8_t>& bytes, Listing& out) {
    const std::string s(bytes.begin(), bytes.end());
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }
    if (parts.size() != 4) {
        return false;
    }
    out.seller = parts[0];
    try {
        out.asset = static_cast<uint8_t>(std::stoul(parts[1]));
        out.price = std::stoull(parts[2]);
        out.height = std::stoull(parts[3]);
    } catch (...) {
        return false;
    }
    return true;
}

bool DeserializeBid(const std::vector<uint8_t>& bytes, Bid& out) {
    const std::string s(bytes.begin(), bytes.end());
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, '|')) {
        parts.push_back(item);
    }
    if (parts.size() != 4) {
        return false;
    }
    out.bidder = parts[0];
    try {
        out.asset = static_cast<uint8_t>(std::stoul(parts[1]));
        out.price = std::stoull(parts[2]);
        out.height = std::stoull(parts[3]);
    } catch (...) {
        return false;
    }
    return true;
}

bool ValidRoyalty(uint16_t royalty_bps) { return royalty_bps <= kMaxRoyaltyBps; }

bool IsDrmOrObl(uint8_t asset) { return asset == kAssetDrm || asset == kAssetObl; }

std::string BalanceKey(const std::string& party, uint8_t asset) {
    return party + "|" + std::to_string(asset);
}

uint64_t DecodeAmount(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return 0;
    try {
        return std::stoull(std::string(bytes.begin(), bytes.end()));
    } catch (...) {
        return 0;
    }
}

void AppendEvent(sidechain::state::StateStore& state, const std::string& payload,
                 uint64_t height) {
    state.AppendEvent(ExecutionDomain::NFT, kNftEventModule,
                      std::to_string(height) + ":" + payload);
}
}  // namespace

WasmRpcService::WasmRpcService(sidechain::wasm::ExecutionEngine& engine,
                               sidechain::state::StateStore& state)
    : engine_(engine), state_(state) {}

ExecutionResult WasmRpcService::DeployContract(const DeployRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::SmartContract, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }

    ExecutionRequest exec;
    exec.domain = ExecutionDomain::SmartContract;
    exec.asset_id = request.asset_id;
    exec.module_id = request.contract_id;
    exec.code = request.init_code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

ExecutionResult WasmRpcService::CallContract(const ContractCall& request) {
    std::string error;
    if (!Validate(ExecutionDomain::SmartContract, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }

    ExecutionRequest exec;
    exec.domain = ExecutionDomain::SmartContract;
    exec.asset_id = request.asset_id;
    exec.module_id = request.contract_id;
    exec.code = request.code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

ExecutionResult WasmRpcService::MintNft(const MintNftRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::NFT, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionResult res;
    if (!ApplyFixedNftCost(request.gas_limit, res)) {
        return res;
    }
    if (state_.Exists(ExecutionDomain::NFT, kNftCoreModule, request.token_id)) {
        res.error = "token exists";
        return res;
    }
    if (request.metadata_hash.empty() || request.canon_reference_hash.empty()) {
        res.error = "invalid canon reference";
        return res;
    }
    if (!ValidRoyalty(request.royalty_bps)) {
        res.error = "invalid royalty_bps";
        return res;
    }
    NftRecord rec;
    rec.owner = request.owner;
    rec.creator = request.creator.empty() ? request.owner : request.creator;
    rec.metadata_hash = request.metadata_hash;
    rec.canon_reference_hash = request.canon_reference_hash;
    rec.mint_height = request.mint_height;
    rec.royalty_bps = request.royalty_bps;

    state_.Put(ExecutionDomain::NFT, kNftCoreModule, request.token_id, Serialize(rec));
    // Legacy slots remain for compatibility with existing viewers.
    std::vector<uint8_t> owner_bytes(request.owner.begin(), request.owner.end());
    std::vector<uint8_t> meta_bytes(request.metadata_hash.begin(), request.metadata_hash.end());
    state_.Put(ExecutionDomain::NFT, kNftModule, request.token_id, owner_bytes);
    state_.Put(ExecutionDomain::NFT, kNftMetaModule, request.token_id, meta_bytes);
    AppendEvent(state_, "NFT_MINTED:" + request.token_id + ":" + rec.creator + ":" +
                             std::to_string(rec.royalty_bps),
                request.mint_height);
    res.success = true;
    return res;
}

ExecutionResult WasmRpcService::TransferNft(const TransferNftRequest& request) {
    std::string error;
    if (!Validate(ExecutionDomain::NFT, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionResult res;
    if (!ApplyFixedNftCost(request.gas_limit, res)) {
        return res;
    }
    if (!state_.Exists(ExecutionDomain::NFT, kNftCoreModule, request.token_id)) {
        res.error = "token missing";
        return res;
    }
    const auto stored = state_.Get(ExecutionDomain::NFT, kNftCoreModule, request.token_id);
    NftRecord rec{};
    if (!Deserialize(stored, rec)) {
        res.error = "corrupt token";
        return res;
    }
    if (rec.owner != request.from) {
        res.error = "ownership mismatch";
        return res;
    }
    rec.owner = request.to;
    state_.Put(ExecutionDomain::NFT, kNftCoreModule, request.token_id, Serialize(rec));
    std::vector<uint8_t> new_owner(request.to.begin(), request.to.end());
    state_.Put(ExecutionDomain::NFT, kNftModule, request.token_id, new_owner);
    AppendEvent(state_, "NFT_TRANSFERRED:" + request.token_id + ":" + request.from + ":" +
                             request.to,
                request.height);
    res.success = true;
    return res;
}

ExecutionResult WasmRpcService::ListNft(const ListNftRequest& request) {
    ExecutionResult res;
    if (!IsDrmOrObl(request.payment_asset)) {
        res.error = "payment must be DRM or OBL";
        return res;
    }
    if (!state_.Exists(ExecutionDomain::NFT, kNftCoreModule, request.token_id)) {
        res.error = "token missing";
        return res;
    }
    const auto stored = state_.Get(ExecutionDomain::NFT, kNftCoreModule, request.token_id);
    NftRecord rec{};
    if (!Deserialize(stored, rec)) {
        res.error = "corrupt token";
        return res;
    }
    if (rec.owner != request.seller) {
        res.error = "ownership mismatch";
        return res;
    }
    Listing listing{request.seller, request.payment_asset, request.price, request.height};
    state_.Put(ExecutionDomain::NFT, kMarketListingModule, request.token_id,
               SerializePriceEntry(listing, listing.seller));
    AppendEvent(state_,
                "NFT_LISTED:" + request.token_id + ":" + request.seller + ":" +
                    std::to_string(request.payment_asset) + ":" + std::to_string(request.price),
                request.height);
    res.success = true;
    res.gas_used = kFixedNftGas;
    return res;
}

ExecutionResult WasmRpcService::PlaceBid(const PlaceBidRequest& request) {
    ExecutionResult res;
    if (!IsDrmOrObl(request.payment_asset)) {
        res.error = "payment must be DRM or OBL";
        return res;
    }
    const auto listing_bytes =
        state_.Get(ExecutionDomain::NFT, kMarketListingModule, request.token_id);
    if (!listing_bytes.empty()) {
        Listing listing{};
        if (DeserializeListing(listing_bytes, listing) && listing.asset != request.payment_asset) {
            res.error = "asset mismatch";
            return res;
        }
    }
    Bid bid{request.bidder, request.payment_asset, request.price, request.height};
    const std::string key = request.token_id + "|" + request.bidder;
    state_.Put(ExecutionDomain::NFT, kMarketBidModule, key, SerializePriceEntry(bid, bid.bidder));
    AppendEvent(state_, "NFT_BID_PLACED:" + request.token_id + ":" + request.bidder + ":" +
                             std::to_string(request.payment_asset) + ":" +
                             std::to_string(request.price),
                request.height);
    res.success = true;
    res.gas_used = kFixedNftGas;
    return res;
}

ExecutionResult WasmRpcService::SettleSale(const SettleSaleRequest& request) {
    ExecutionResult res;
    if (!IsDrmOrObl(request.payment_asset)) {
        res.error = "payment must be DRM or OBL";
        return res;
    }
    const auto stored = state_.Get(ExecutionDomain::NFT, kNftCoreModule, request.token_id);
    NftRecord rec{};
    if (!Deserialize(stored, rec)) {
        res.error = "token missing";
        return res;
    }

    Listing listing{};
    bool has_listing = false;
    const auto listing_bytes =
        state_.Get(ExecutionDomain::NFT, kMarketListingModule, request.token_id);
    if (!listing_bytes.empty()) {
        if (!DeserializeListing(listing_bytes, listing)) {
            res.error = "corrupt listing";
            return res;
        }
        has_listing = true;
    }

    Bid bid{};
    const std::string bid_key = request.token_id + "|" + request.buyer;
    const auto bid_bytes = state_.Get(ExecutionDomain::NFT, kMarketBidModule, bid_key);
    bool has_bid = false;
    if (!bid_bytes.empty()) {
        if (!DeserializeBid(bid_bytes, bid)) {
            res.error = "corrupt bid";
            return res;
        }
        has_bid = true;
    }

    uint8_t sale_asset = request.payment_asset;
    uint64_t sale_price = request.price;
    std::string seller = rec.owner;

    if (has_listing) {
        if (listing.seller != seller) {
            res.error = "ownership mismatch";
            return res;
        }
        if (sale_price == 0) {
            sale_price = listing.price;
        }
        if (sale_asset != listing.asset || sale_price != listing.price) {
            res.error = "listing terms mismatch";
            return res;
        }
    } else if (has_bid) {
        if (sale_price == 0) {
            sale_price = bid.price;
        }
        if (sale_asset != bid.asset || sale_price != bid.price) {
            res.error = "bid terms mismatch";
            return res;
        }
    } else {
        res.error = "no listing or bid";
        return res;
    }

    const uint64_t royalty_amount = (sale_price * rec.royalty_bps) / 10'000;
    const uint64_t seller_amount = sale_price - royalty_amount;

    auto credit = [&](const std::string& party, uint64_t amount) {
        const auto key = BalanceKey(party, sale_asset);
        const auto existing = DecodeAmount(
            state_.Get(ExecutionDomain::NFT, kMarketBalanceModule, key));
        const uint64_t updated = existing + amount;
        const std::string encoded = std::to_string(updated);
        state_.Put(ExecutionDomain::NFT, kMarketBalanceModule, key,
                   std::vector<uint8_t>(encoded.begin(), encoded.end()));
    };

    credit(rec.creator, royalty_amount);
    credit(seller, seller_amount);

    rec.owner = request.buyer;
    state_.Put(ExecutionDomain::NFT, kNftCoreModule, request.token_id, Serialize(rec));
    std::vector<uint8_t> new_owner(request.buyer.begin(), request.buyer.end());
    state_.Put(ExecutionDomain::NFT, kNftModule, request.token_id, new_owner);

    state_.Put(ExecutionDomain::NFT, kMarketListingModule, request.token_id, {});
    state_.Put(ExecutionDomain::NFT, kMarketBidModule, bid_key, {});

    AppendEvent(
        state_,
        "NFT_SALE_SETTLED:" + request.token_id + ":" + seller + ":" + request.buyer + ":" +
            std::to_string(sale_asset) + ":" + std::to_string(sale_price) + ":" +
            std::to_string(royalty_amount) + ":" + std::to_string(seller_amount),
        request.height);

    res.success = true;
    res.gas_used = kFixedNftGas;
    return res;
}

ExecutionResult WasmRpcService::CallDapp(const DappCall& request) {
    std::string error;
    if (!Validate(ExecutionDomain::Dapp, request.asset_id, error)) {
        ExecutionResult res;
        res.error = error;
        return res;
    }
    ExecutionRequest exec;
    exec.domain = ExecutionDomain::Dapp;
    exec.asset_id = request.asset_id;
    exec.module_id = request.app_id;
    exec.code = request.code;
    exec.gas_limit = request.gas_limit;
    return engine_.Execute(exec, state_);
}

}  // namespace sidechain::rpc
