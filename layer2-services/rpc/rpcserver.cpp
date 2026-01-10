#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include <openssl/sha.h>

#include "rpcserver.h"
#include "../../layer1-core/consensus/params.h"
#include "../../layer1-core/tx/transaction.h"
#include "../../sidechain/wasm/runtime/types.h"

namespace http = boost::beast::http;

namespace rpc {

namespace {
std::unordered_map<std::string, std::string> ParseKeyValues(const std::string& raw)
{
    // Maximum allowed key-value pairs: 100
    const size_t MAX_KV_PAIRS = 100;
    
    // Maximum allowed key or value length: 64KB
    const size_t MAX_KV_LENGTH = 64 * 1024;
    
    // Maximum input size: 1MB
    const size_t MAX_INPUT_SIZE = 1 * 1024 * 1024;
    
    if (raw.size() > MAX_INPUT_SIZE) {
        throw std::runtime_error("Input too large for ParseKeyValues");
    }
    
    std::unordered_map<std::string, std::string> kv;
    std::string cleaned = raw;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '"'), cleaned.end());
    std::stringstream ss(cleaned);
    std::string part;
    while (std::getline(ss, part, ';')) {
        if (kv.size() >= MAX_KV_PAIRS) {
            throw std::runtime_error("Too many key-value pairs");
        }
        
        auto pos = part.find('=');
        if (pos == std::string::npos) continue;
        auto key = part.substr(0, pos);
        auto value = part.substr(pos + 1);
        
        if (key.size() > MAX_KV_LENGTH || value.size() > MAX_KV_LENGTH) {
            throw std::runtime_error("Key or value too long");
        }
        
        if (!key.empty()) kv[key] = value;
    }
    return kv;
}

std::vector<sidechain::wasm::Instruction> DecodeInstructions(const std::string& hex)
{
    // Maximum allowed hex string size: 1MB (500KB of instructions)
    const size_t MAX_HEX_SIZE = 1 * 1024 * 1024;
    
    // Maximum allowed number of instructions: 100,000
    const size_t MAX_INSTRUCTIONS = 100000;
    
    if (hex.size() > MAX_HEX_SIZE) {
        throw std::runtime_error("Instruction hex string too large");
    }
    
    std::vector<sidechain::wasm::Instruction> out;
    auto cleaned = hex;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '"'), cleaned.end());
    
    // Validate hex string format
    if (cleaned.size() % 2 != 0) {
        throw std::runtime_error("Invalid hex string: odd length");
    }
    
    // Each instruction is 5 bytes (1 byte opcode + 4 bytes immediate)
    // Fail fast if instruction count exceeds limit before expensive hex decoding
    if (cleaned.size() % 10 != 0) {
        throw std::runtime_error("Invalid instruction data: size not multiple of 10 hex chars (5 bytes)");
    }
    const size_t instructionCount = cleaned.size() / 10;
    if (instructionCount > MAX_INSTRUCTIONS) {
        throw std::runtime_error("Too many instructions");
    }
    
    // Pre-allocate bytes vector to avoid reallocations
    std::vector<uint8_t> bytes;
    bytes.reserve(cleaned.size() / 2);
    
    // Optimize: use lookup table for hex conversion instead of stoi
    // Lookup table indexed by ASCII value, returns hex digit value or -1 for invalid
    static const int8_t hex_lut[256] = {
        // Control characters (0x00-0x1F)
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Space and symbols (0x20-0x2F)
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Digits 0-9 (0x30-0x39)
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
        // Uppercase letters: @ABCDEFGHIJKLMNO (0x40-0x4F)
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Uppercase letters: PQRSTUVWXYZ[\]^_ (0x50-0x5F)
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Lowercase letters: `abcdefghijklmno (0x60-0x6F)
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Lowercase letters: pqrstuvwxyz{|}~DEL (0x70-0x7F)
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        // Extended ASCII (0x80-0xFF) - all invalid
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    for (size_t i = 0; i + 2 <= cleaned.size(); i += 2) {
        int8_t high = hex_lut[static_cast<uint8_t>(cleaned[i])];
        int8_t low = hex_lut[static_cast<uint8_t>(cleaned[i + 1])];
        if (high < 0 || low < 0) {
            throw std::runtime_error("Invalid hex character in instruction data");
        }
        bytes.push_back(static_cast<uint8_t>((high << 4) | low));
    }
    
    if (bytes.empty()) return out;
    
    // Pre-allocate output vector
    out.reserve(instructionCount);
    
    for (size_t i = 0; i + 5 <= bytes.size(); i += 5) {
        sidechain::wasm::Instruction ins;
        ins.op = static_cast<sidechain::wasm::OpCode>(bytes[i]);
        int32_t imm = 0;
        std::memcpy(&imm, bytes.data() + i + 1, sizeof(imm));
        ins.immediate = imm;
        out.push_back(ins);
    }
    return out;
}

// Shared hex encoding utility - more efficient than multiple implementations
inline std::string EncodeHex(const std::vector<uint8_t>& data) {
    static const char* hex_table = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(hex_table[b >> 4]);
        out.push_back(hex_table[b & 0xf]);
    }
    return out;
}

std::string JsonEscape(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (unsigned char c : input) {
        switch (c) {
            case '\"':
                out.append("\\\"");
                break;
            case '\\':
                out.append("\\\\");
                break;
            case '\b':
                out.append("\\b");
                break;
            case '\f':
                out.append("\\f");
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\t':
                out.append("\\t");
                break;
            default:
                if (c < 0x20) {
                    char buffer[7];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", c);
                    out.append(buffer);
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    return out;
}
} // namespace

std::string FormatExecResult(const sidechain::wasm::ExecutionResult& res)
{
    std::stringstream ss;
    ss << "{\"success\":" << (res.success ? "true" : "false")
       << ",\"gas_used\":" << res.gas_used
       << ",\"state_writes\":" << res.state_writes
       << ",\"output\":\"" << EncodeHex(res.output) << "\"";
    if (!res.error.empty()) {
        ss << ",\"error\":\"" << JsonEscape(res.error) << "\"";
    }
    ss << "}";
    return ss.str();
}

static Block DeserializeBlock(const std::vector<uint8_t>& buf)
{
    Block block{};
    if (buf.size() < sizeof(BlockHeader)) return block;

    std::memcpy(&block.header, buf.data(), sizeof(BlockHeader));
    size_t offset = sizeof(BlockHeader);

    if (offset + sizeof(uint32_t) > buf.size()) return block;
    uint32_t txCount{0};
    std::memcpy(&txCount, buf.data() + offset, sizeof(txCount));
    offset += sizeof(txCount);

    block.transactions.reserve(txCount);
    for (uint32_t i = 0; i < txCount && offset < buf.size(); ++i) {
        if (offset + sizeof(uint32_t) > buf.size()) break;
        uint32_t len{0};
        std::memcpy(&len, buf.data() + offset, sizeof(len));
        offset += sizeof(len);
        if (offset + len > buf.size()) break;
        std::vector<uint8_t> txBytes(buf.begin() + offset, buf.begin() + offset + len);
        offset += len;
        try {
            block.transactions.push_back(DeserializeTransaction(txBytes));
        } catch (...) {
            break;
        }
    }
    return block;
}

RPCServer::RPCServer(boost::asio::io_context& io, const std::string& user, const std::string& pass, uint16_t port)
    : m_io(io), m_acceptor(io, {boost::asio::ip::tcp::v4(), port}), m_user(user), m_pass(pass)
{
}

void RPCServer::SetBlockStorePath(std::string path)
{
    m_blockPath = std::move(path);
}

void RPCServer::AttachCoreHandlers(mempool::Mempool& pool, wallet::WalletBackend& wallet, txindex::TxIndex& index, net::P2PNode& p2p)
{
    auto formatBalances = [](const std::unordered_map<uint8_t, uint64_t>& balances) {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& policy : consensus::GetAllAssetPolicies()) {
            auto it = balances.find(policy.assetId);
            uint64_t value = it == balances.end() ? 0 : it->second;
            if (!first) ss << ",";
            ss << "\"" << consensus::AssetSymbol(policy.assetId) << "\":" << value;
            first = false;
        }
        ss << "}";
        return ss.str();
    };

    auto parseAssetParam = [](const std::string& param, uint8_t& out) {
        if (param.empty() || param == "null")
            return false;
        if (consensus::ParseAssetSymbol(param, out))
            return true;
        try {
            uint8_t candidate = static_cast<uint8_t>(std::stoul(param));
            if (IsValidAssetId(candidate)) {
                out = candidate;
                return true;
            }
        } catch (...) {
        }
        return false;
    };

    pool.SetOnAccept([&p2p](const Transaction& tx) {
        auto payload = Serialize(tx);
        p2p.Broadcast(net::Message{"tx", payload});
    });

    Register("getbalance", [&wallet, &formatBalances, &parseAssetParam](const std::string& params) {
        auto trimmed = TrimQuotes(params);
        if (!trimmed.empty() && trimmed != "null") {
            uint8_t asset{0};
            if (!parseAssetParam(trimmed, asset))
                return std::string("null");
            return std::to_string(wallet.GetBalance(asset));
        }
        return formatBalances(wallet.GetBalances());
    });

    Register("getblockcount", [&index](const std::string&) {
        return std::to_string(index.BlockCount());
    });

    Register("gettransaction", [&index](const std::string& params) {
        auto hash = ParseHash(params);
        uint32_t height{0};
        bool found = index.Lookup(hash, height);
        return std::string("{\"found\":") + (found ? "true" : "false") + ",\"height\":" + std::to_string(height) + "}";
    });

    Register("getrawtransaction", [&index, this](const std::string& params) {
        auto hash = ParseHash(params);
        std::stringstream ss;
        uint32_t height{0};
        if (!index.Lookup(hash, height)) return std::string("null");
        auto blk = ReadBlock(height);
        if (!blk) return std::string("null");
        for (const auto& tx : blk->transactions) {
            if (tx.GetHash() == hash) {
                ss << '"' << HexEncode(Serialize(tx)) << '"';
                return ss.str();
            }
        }
        return std::string("null");
    });

    Register("getutxos", [&wallet, &formatBalances, &parseAssetParam](const std::string& params) {
        auto trimmed = TrimQuotes(params);
        if (!trimmed.empty() && trimmed != "null") {
            uint8_t asset{0};
            if (!parseAssetParam(trimmed, asset))
                return std::string("null");
            return std::to_string(wallet.GetBalance(asset));
        }
        return formatBalances(wallet.GetBalances());
    });

    Register("estimatefee", [&pool](const std::string& params) {
        size_t percentile = 50;
        try { percentile = std::stoul(TrimQuotes(params)); } catch (...) {}
        return std::to_string(pool.EstimateFeeRate(percentile));
    });

    Register("sendtx", [&pool, &p2p](const std::string& params) {
        auto hex = TrimQuotes(params);
        auto raw = ParseHex(hex);
        Transaction tx = DeserializeTransaction(raw);
        uint64_t fee = 0; // rely on caller to include fee in inputs/outputs difference
        bool ok = pool.Accept(tx, fee);
        if (ok) {
            auto payload = Serialize(tx);
            p2p.Broadcast(net::Message{"tx", payload});
        }
        return std::string("{\"accepted\":") + (ok ? "true" : "false") + "}";
    });

    Register("sendrawtransaction", [this](const std::string& params) {
        return GetHandler("sendtx")(params);
    });

    Register("getstakinginfo", [&wallet](const std::string&) {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& policy : consensus::GetAllAssetPolicies()) {
            if (!first) ss << ",";
            ss << "\"" << consensus::AssetSymbol(policy.assetId) << "\":{";
            ss << "\"posAllowed\":false,";
            ss << "\"balance\":" << wallet.GetBalance(policy.assetId) << "}";
            first = false;
        }
        ss << "}";
        return ss.str();
    });

    Register("getassetpolicy", [&](const std::string& params) {
        auto trimmed = TrimQuotes(params);
        std::stringstream ss;
        ss << "{";
        bool first = true;
        for (const auto& policy : consensus::GetAllAssetPolicies()) {
            if (!first) ss << ",";
            ss << "\"" << consensus::AssetSymbol(policy.assetId) << "\":{";
            ss << "\"id\":" << static_cast<int>(policy.assetId) << ",";
            ss << "\"powAllowed\":" << (policy.powAllowed ? "true" : "false") << ",";
            ss << "\"posAllowed\":false,";
            ss << "\"halvingInterval\":" << policy.powHalvingInterval << ",";
            ss << "\"initialSubsidy\":" << policy.powInitialSubsidy << ",";
            ss << "\"maxMoney\":" << policy.maxMoney << "}";
            first = false;
        }
        if (!trimmed.empty() && trimmed != "null") {
            uint8_t asset{0};
            if (parseAssetParam(trimmed, asset)) {
                ss << ",\"active\":" << static_cast<int>(asset);
            }
        }
        ss << "}";
        return ss.str();
    });
}

void RPCServer::AttachBridgeHandlers(crosschain::BridgeManager& bridge)
{
    Register("createbridgelock", [&bridge, this](const std::string& params) {
        std::stringstream ss(TrimQuotes(params));
        std::string chain, txid, destination, secretHex, privHex;
        uint64_t amount{0};
        uint64_t timeout{0};
        std::getline(ss, chain, ',');
        std::getline(ss, txid, ',');
        std::getline(ss, destination, ',');
        ss >> amount;
        ss.ignore(1);
        std::getline(ss, secretHex, ',');
        ss >> timeout;
        ss.ignore(1);
        std::getline(ss, privHex, ',');

        auto secret = ParseHex(secretHex);
        std::array<uint8_t, 32> secretHash{};
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
        if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestInit_ex failed");
        }
        if (EVP_DigestUpdate(ctx, secret.data(), secret.size()) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestUpdate failed");
        }
        unsigned int hashLen = 32;
        if (EVP_DigestFinal_ex(ctx, secretHash.data(), &hashLen) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("EVP_DigestFinal_ex failed");
        }
        EVP_MD_CTX_free(ctx);

        std::array<uint8_t, 32> priv{};
        auto privVec = ParseHex(privHex);
        std::copy_n(privVec.begin(), std::min<size_t>(privVec.size(), 32), priv.begin());

        std::vector<uint8_t> sig;
        auto lock = bridge.InitiateOutboundLock(chain, txid, destination, amount, secretHash, timeout, priv, sig);
        std::stringstream out;
        out << "{\"lockId\":\"" << lock.id << "\",\"signature\":\"" << HexEncode(sig) << "\"}";
        return out.str();
    });

    Register("claimbridge", [&bridge, this](const std::string& params) {
        std::stringstream ss(TrimQuotes(params));
        std::string lockId, secretHex;
        uint64_t height{0};
        std::getline(ss, lockId, ',');
        std::getline(ss, secretHex, ',');
        ss >> height;
        auto secret = ParseHex(secretHex);
        std::vector<uint8_t> sig;
        bool ok = bridge.Claim(lockId, secret, height, sig);
        std::stringstream out;
        out << "{\"claimed\":" << (ok ? "true" : "false") << ",\"signature\":\"" << HexEncode(sig) << "\"}";
        return out.str();
    });

    Register("refundbridge", [&bridge](const std::string& params) {
        std::stringstream ss(TrimQuotes(params));
        std::string lockId;
        uint64_t height{0};
        std::getline(ss, lockId, ',');
        ss >> height;
        bool ok = bridge.Refund(lockId, height);
        return std::string("{\"refunded\":") + (ok ? "true" : "false") + "}";
    });
}

void RPCServer::AttachSidechainHandlers(sidechain::rpc::WasmRpcService& wasm)
{
    using namespace sidechain::wasm;
    Register("deploy_contract", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::DeployRequest req;
        req.contract_id = kv["module"];
        req.asset_id = kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetDrm;
        req.gas_limit = kv.count("gas") ? std::stoull(kv["gas"]) : 0;
        req.init_code = DecodeInstructions(kv["code"]);
        return FormatExecResult(wasm.DeployContract(req));
    });

    Register("call_contract", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::ContractCall req;
        req.contract_id = kv["module"];
        req.asset_id = kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetDrm;
        req.gas_limit = kv.count("gas") ? std::stoull(kv["gas"]) : 0;
        req.code = DecodeInstructions(kv["code"]);
        return FormatExecResult(wasm.CallContract(req));
    });

    Register("mint_nft", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::MintNftRequest req;
        req.token_id = kv["token"];
        req.owner = kv["owner"];
        req.metadata_hash = kv["meta"];
        req.creator = kv.count("creator") ? kv["creator"] : req.owner;
        req.canon_reference_hash =
            kv.count("canon") ? kv["canon"] : req.metadata_hash;
        req.royalty_bps = kv.count("royalty") ? static_cast<uint16_t>(std::stoul(kv["royalty"])) : 0;
        req.mint_height = kv.count("height") ? std::stoull(kv["height"]) : 0;
        req.asset_id = kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetTln;
        req.gas_limit = kv.count("gas") ? std::stoull(kv["gas"]) : 0;
        return FormatExecResult(wasm.MintNft(req));
    });

    Register("transfer_nft", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::TransferNftRequest req;
        req.token_id = kv["token"];
        req.from = kv["from"];
        req.to = kv["to"];
        req.asset_id = kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetTln;
        req.gas_limit = kv.count("gas") ? std::stoull(kv["gas"]) : 0;
        req.height = kv.count("height") ? std::stoull(kv["height"]) : 0;
        return FormatExecResult(wasm.TransferNft(req));
    });

    Register("list_nft", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::ListNftRequest req;
        req.token_id = kv["token"];
        req.seller = kv["seller"];
        req.payment_asset =
            kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetDrm;
        req.price = kv.count("price") ? std::stoull(kv["price"]) : 0;
        req.height = kv.count("height") ? std::stoull(kv["height"]) : 0;
        return FormatExecResult(wasm.ListNft(req));
    });

    Register("place_nft_bid", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::PlaceBidRequest req;
        req.token_id = kv["token"];
        req.bidder = kv["bidder"];
        req.payment_asset =
            kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetDrm;
        req.price = kv.count("price") ? std::stoull(kv["price"]) : 0;
        req.height = kv.count("height") ? std::stoull(kv["height"]) : 0;
        return FormatExecResult(wasm.PlaceBid(req));
    });

    Register("settle_nft_sale", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::SettleSaleRequest req;
        req.token_id = kv["token"];
        req.buyer = kv["buyer"];
        req.payment_asset =
            kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetDrm;
        req.price = kv.count("price") ? std::stoull(kv["price"]) : 0;
        req.height = kv.count("height") ? std::stoull(kv["height"]) : 0;
        return FormatExecResult(wasm.SettleSale(req));
    });

    Register("call_dapp", [&wasm](const std::string& params) {
        auto kv = ParseKeyValues(params);
        sidechain::rpc::DappCall req;
        req.app_id = kv["app"];
        req.asset_id = kv.count("asset") ? static_cast<uint8_t>(std::stoul(kv["asset"])) : kAssetObl;
        req.gas_limit = kv.count("gas") ? std::stoull(kv["gas"]) : 0;
        req.code = DecodeInstructions(kv["code"]);
        return FormatExecResult(wasm.CallDapp(req));
    });
}

void RPCServer::Register(const std::string& method, Handler handler)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_handlers[method] = std::move(handler);
}

void RPCServer::Start()
{
    Accept();
}

void RPCServer::Stop()
{
    boost::system::error_code ec;
    m_acceptor.close(ec);
}

void RPCServer::Accept()
{
    m_acceptor.async_accept([this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
        if (!ec) HandleSession(std::move(socket));
        Accept();
    });
}

void RPCServer::HandleSession(boost::asio::ip::tcp::socket socket)
{
    auto ownedSocket = std::make_shared<boost::asio::ip::tcp::socket>(std::move(socket));
    auto remote = ownedSocket->remote_endpoint().address().to_string();
    auto buf = std::make_shared<boost::beast::flat_buffer>();
    auto req = std::make_shared<http::request<http::string_body>>();
    http::async_read(*ownedSocket, *buf, *req, [this, buf, req, remote, ownedSocket](const boost::system::error_code& ec, std::size_t) mutable {
        if (!ec) {
            auto resp = Process(*req, remote);
            auto sp = std::make_shared<http::response<http::string_body>>(std::move(resp));
            http::async_write(*ownedSocket, *sp, [ownedSocket, sp](const boost::system::error_code&, std::size_t) {});
        }
    });
}

bool RPCServer::RateLimit(const std::string& remote)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto now = std::chrono::steady_clock::now();
    auto& bucket = m_rate[remote];
    if (now - bucket.second > std::chrono::minutes(1)) {
        bucket = {0, now};
    }
    bucket.first++;
    return bucket.first < 120; // 2 rps
}

http::response<http::string_body> RPCServer::Process(const http::request<http::string_body>& req, const std::string& remote)
{
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(false);

    if (!RateLimit(remote)) {
        res.result(http::status::too_many_requests);
        res.body() = "{\"error\":\"rate limited\"}";
        return res;
    }

    auto auth = req[http::field::authorization];
    if (!CheckAuth(std::string(auth)) && !CheckToken(req)) {
        res.result(http::status::unauthorized);
        res.body() = "{\"error\":\"auth required\"}";
        return res;
    }

    auto [method, params] = ParseJsonRpc(req.body());
    auto handler = GetHandler(method);
    if (!handler) {
        res.result(http::status::bad_request);
        res.body() = "{\"error\":\"unknown method\"}";
        return res;
    }

    try {
        auto result = handler(params);
        res.body() = std::string("{\"result\":") + result + "}";
    } catch (const std::exception& ex) {
        res.result(http::status::internal_server_error);
        res.body() = std::string("{\"error\":\"") + ex.what() + "\"}";
    }
    return res;
}

bool RPCServer::CheckAuth(const std::string& header) const
{
    if (header.rfind("Basic ", 0) != 0) return false;
    auto encoded = header.substr(6);
    std::string decoded;
    try {
        decoded.resize(boost::beast::detail::base64::decoded_size(encoded.size()));
        auto len = boost::beast::detail::base64::decode(&decoded[0], encoded.data(), encoded.size());
        decoded.resize(len.first);
    } catch (...) {
        return false;
    }
    auto pos = decoded.find(':');
    if (pos == std::string::npos) return false;
    auto user = decoded.substr(0, pos);
    auto pass = decoded.substr(pos + 1);
    return user == m_user && pass == m_pass;
}

bool RPCServer::CheckToken(const http::request<http::string_body>& req) const
{
    auto token = req["X-Auth-Token"];
    return token == m_token;
}

RPCServer::Handler RPCServer::GetHandler(const std::string& name)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_handlers.find(name);
    if (it == m_handlers.end()) return {};
    return it->second;
}

std::string RPCServer::HexEncode(const std::vector<uint8_t>& data)
{
    return EncodeHex(data);
}

std::optional<Block> RPCServer::ReadBlock(uint32_t height)
{
    // Improved block retrieval using indexed access via height->offset mapping.
    // Block index file (.idx) contains height->file_offset pairs for O(1) lookups
    // instead of the previous O(n) linear scan through all blocks.
    
    // Load block index if not already cached
    std::ifstream indexFile(m_blockPath + ".idx", std::ios::binary);
    if (!indexFile.is_open()) {
        // Fallback: If index doesn't exist, try legacy linear scan for backwards compatibility
        // This ensures the RPC server can still read old block files without an index.
        std::ifstream file(m_blockPath, std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        
        while (file.peek() != EOF) {
            uint32_t h = 0; uint32_t len = 0;
            file.read(reinterpret_cast<char*>(&h), sizeof(h));
            file.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (!file || len > 100*1024*1024) break; // Sanity check: block size < 100MB
            std::vector<uint8_t> buf(len);
            file.read(reinterpret_cast<char*>(buf.data()), len);
            if (h == height) {
                return DeserializeBlock(buf);
            }
        }
        return std::nullopt;
    }
    
    // Read index to find block offset
    // Index file format: [count] [height, offset] pairs
    // Heights are stored in ascending order, allowing binary search
    uint32_t indexCount = 0;
    indexFile.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
    if (!indexFile || indexCount > 10000000) return std::nullopt; // Sanity check
    
    // Binary search through index entries for O(log n) lookup
    // This is more efficient than O(n) linear scan for large blockchains
    std::optional<uint64_t> blockOffset;
    uint32_t left = 0, right = indexCount;
    
    while (left < right) {
        uint32_t mid = left + (right - left) / 2;
        indexFile.seekg(sizeof(uint32_t) + mid * (sizeof(uint32_t) + sizeof(uint64_t)));
        
        uint32_t h = 0;
        uint64_t offset = 0;
        indexFile.read(reinterpret_cast<char*>(&h), sizeof(h));
        indexFile.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        
        if (!indexFile) break;
        
        if (h == height) {
            blockOffset = offset;
            break;
        } else if (h < height) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    
    if (!blockOffset) return std::nullopt;
    
    // Seek to block offset and read
    std::ifstream blockFile(m_blockPath, std::ios::binary);
    if (!blockFile.is_open()) return std::nullopt;
    
    blockFile.seekg(*blockOffset);
    if (!blockFile.good()) return std::nullopt;
    
    uint32_t blockSize = 0;
    blockFile.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
    if (!blockFile || blockSize == 0 || blockSize > 100*1024*1024) return std::nullopt;
    
    std::vector<uint8_t> buf(blockSize);
    blockFile.read(reinterpret_cast<char*>(buf.data()), blockSize);
    if (!blockFile) return std::nullopt;
    
    return DeserializeBlock(buf);
}

std::vector<uint8_t> RPCServer::ParseHex(const std::string& hex)
{
    // Maximum allowed hex string size: 1MB (512KB binary data)
    const size_t MAX_HEX_SIZE = 1 * 1024 * 1024;
    
    if (hex.size() > MAX_HEX_SIZE) {
        throw std::runtime_error("Hex string too large");
    }
    
    // Validate hex string has even length
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("Invalid hex string: odd length");
    }
    
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        try {
            uint8_t byte = std::stoi(hex.substr(i, 2), nullptr, 16);
            out.push_back(byte);
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Invalid hex character");
        } catch (const std::out_of_range&) {
            throw std::runtime_error("Hex value out of range");
        }
    }
    return out;
}

uint256 RPCServer::ParseHash(const std::string& params)
{
    auto hex = TrimQuotes(params);
    auto raw = ParseHex(hex);
    uint256 h{};
    std::copy(raw.begin(), raw.begin() + std::min<size_t>(raw.size(), h.size()), h.begin());
    return h;
}

std::string RPCServer::TrimQuotes(std::string in)
{
    in.erase(std::remove(in.begin(), in.end(), '"'), in.end());
    return in;
}

std::pair<std::string, std::string> RPCServer::ParseJsonRpc(const std::string& body)
{
    // Maximum allowed request body size: 10MB (prevents memory exhaustion attacks)
    const size_t MAX_REQUEST_SIZE = 10 * 1024 * 1024;
    
    // Maximum allowed method name length: 128 bytes
    const size_t MAX_METHOD_LENGTH = 128;
    
    // Maximum allowed params length: 1MB
    const size_t MAX_PARAMS_LENGTH = 1 * 1024 * 1024;
    
    // Validate total request size
    if (body.size() > MAX_REQUEST_SIZE) {
        throw std::runtime_error("Request too large");
    }
    
    // Simple parser without regex for better performance: {"method":"name","params":"value"}
    // Note: This is a lightweight parser for well-formed RPC requests. For robustness,
    // a full JSON library should be used in production.
    std::string method, params;
    
    auto findStringValue = [&body](const std::string& key, size_t startPos = 0) -> std::string {
        size_t keyPos = body.find(key, startPos);
        if (keyPos == std::string::npos) return "";
        size_t colonPos = body.find(':', keyPos);
        if (colonPos == std::string::npos) return "";
        size_t startQuote = body.find('"', colonPos);
        if (startQuote == std::string::npos) return "";
        
        // Simple escape handling: count consecutive backslashes before quote
        size_t endQuote = startQuote + 1;
        while (endQuote < body.size()) {
            if (body[endQuote] == '"') {
                // Count backslashes before this quote
                size_t backslashCount = 0;
                size_t check = endQuote - 1;
                while (check > startQuote && body[check] == '\\') {
                    backslashCount++;
                    if (check == 0) break;
                    check--;
                }
                // If even number of backslashes, quote is not escaped
                if (backslashCount % 2 == 0) {
                    return body.substr(startQuote + 1, endQuote - startQuote - 1);
                }
            }
            endQuote++;
        }
        return "";
    };
    
    method = findStringValue("\"method\"");
    
    // Validate method length
    if (method.size() > MAX_METHOD_LENGTH) {
        throw std::runtime_error("Method name too long");
    }
    
    // Find params - can be string, number, object, or array
    size_t paramsPos = body.find("\"params\"");
    if (paramsPos != std::string::npos) {
        size_t colonPos = body.find(':', paramsPos);
        if (colonPos != std::string::npos) {
            size_t start = colonPos + 1;
            while (start < body.size() && std::isspace(body[start])) ++start;
            if (start < body.size()) {
                // Simple extraction: find comma or closing brace at depth 0
                int depth = 0;
                bool inString = false;
                size_t end = start;
                for (size_t i = start; i < body.size(); ++i) {
                    if (body[i] == '"') {
                        // Check if quote is escaped
                        size_t backslashCount = 0;
                        if (i > 0) {
                            size_t check = i - 1;
                            while (check > 0 && body[check] == '\\') {
                                backslashCount++;
                                check--;
                            }
                        }
                        if (backslashCount % 2 == 0) {
                            inString = !inString;
                        }
                    } else if (!inString) {
                        if (body[i] == '{' || body[i] == '[') depth++;
                        else if (body[i] == '}' || body[i] == ']') {
                            if (depth > 0) depth--;
                            else { end = i; break; }
                        }
                        else if ((body[i] == ',' || body[i] == '}') && depth == 0) {
                            end = i;
                            break;
                        }
                    }
                }
                if (end > start) {
                    params = body.substr(start, end - start);
                    
                    // Validate params length
                    if (params.size() > MAX_PARAMS_LENGTH) {
                        throw std::runtime_error("Params too long");
                    }
                }
            }
        }
    }
    
    return {method, params};
}

} // namespace rpc
