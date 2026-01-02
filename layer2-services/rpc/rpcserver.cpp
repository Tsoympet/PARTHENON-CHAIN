#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <unordered_map>
#include <openssl/sha.h>

#include "rpcserver.h"

namespace http = boost::beast::http;

namespace rpc {

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
        for (const auto& kv : balances) {
            if (!first) ss << ",";
            ss << "\"asset" << static_cast<int>(kv.first) << "\":" << kv.second;
            first = false;
        }
        ss << "}";
        return ss.str();
    };

    pool.SetOnAccept([&p2p](const Transaction& tx) {
        auto payload = Serialize(tx);
        p2p.Broadcast(net::Message{"tx", payload});
    });

    Register("getbalance", [&wallet, &formatBalances](const std::string& params) {
        auto trimmed = TrimQuotes(params);
        if (!trimmed.empty() && trimmed != "null") {
            try {
                uint8_t asset = static_cast<uint8_t>(std::stoul(trimmed));
                return std::to_string(wallet.GetBalance(asset));
            } catch (...) {
                return std::string("null");
            }
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

    Register("getutxos", [&wallet, &formatBalances](const std::string& params) {
        auto trimmed = TrimQuotes(params);
        if (!trimmed.empty() && trimmed != "null") {
            try {
                uint8_t asset = static_cast<uint8_t>(std::stoul(trimmed));
                return std::to_string(wallet.GetBalance(asset));
            } catch (...) {
                return std::string("null");
            }
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
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, secret.data(), secret.size());
        SHA256_Final(secretHash.data(), &ctx);

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
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0xf]);
    }
    return out;
}

std::optional<Block> RPCServer::ReadBlock(uint32_t height)
{
    std::ifstream file(m_blockPath, std::ios::binary);
    if (!file.is_open()) return std::nullopt;
    // naive linear scan example placeholder: blocks serialized length prefixed
    while (file.peek() != EOF) {
        uint32_t h = 0; uint32_t len = 0;
        file.read(reinterpret_cast<char*>(&h), sizeof(h));
        file.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!file) break;
        std::vector<uint8_t> buf(len);
        file.read(reinterpret_cast<char*>(buf.data()), len);
        if (h == height) {
            return DeserializeBlock(buf);
        }
    }
    return std::nullopt;
}

std::vector<uint8_t> RPCServer::ParseHex(const std::string& hex)
{
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t byte = std::stoi(hex.substr(i, 2), nullptr, 16);
        out.push_back(byte);
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
    // very small parser: {"method":"name","params":"value"}
    std::regex methodRe("\"method\"\\s*:\\s*\"([^\"]+)\"");
    std::regex paramsRe("\"params\"\\s*:\\s*([^,}]+)");
    std::smatch m;
    std::string method, params;
    if (std::regex_search(body, m, methodRe)) method = m[1];
    if (std::regex_search(body, m, paramsRe)) params = m[1];
    return {method, params};
}

} // namespace rpc
