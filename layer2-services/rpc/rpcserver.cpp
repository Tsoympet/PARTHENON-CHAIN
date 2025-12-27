#include "rpcserver.h"

#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <algorithm>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <fstream>
#include <mutex>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../mempool/mempool.h"
#include "../net/p2p.h"
#include "../wallet/wallet.h"
#include "../index/txindex.h"
#include "../../layer1-core/block/block.h"
#include "../../layer1-core/tx/transaction.h"

namespace http = boost::beast::http;

namespace rpc {

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
    pool.SetOnAccept([&p2p](const Transaction& tx) {
        auto payload = Serialize(tx);
        p2p.Broadcast(net::Message{"tx", payload});
    });

    Register("getbalance", [&wallet](const std::string&) {
        return std::to_string(wallet.GetBalance());
    });

    Register("getblockcount", [&index](const std::string&) {
        return std::to_string(index.BlockCount());
    });
    void SetBlockStorePath(std::string path) { m_blockPath = std::move(path); }

    void AttachCoreHandlers(mempool::Mempool& pool, wallet::WalletBackend& wallet, txindex::TxIndex& index, net::P2PNode& p2p)
    {
        pool.SetOnAccept([&p2p](const Transaction& tx) {
            auto payload = Serialize(tx);
            p2p.Broadcast(net::Message{"tx", payload});
        });

        Register("getbalance", [&wallet](const std::string&) {
            return std::to_string(wallet.GetBalance());
        });

        Register("getblockcount", [&index](const std::string&) {
            return std::to_string(index.BlockCount());
        });

        Register("getblock", [this, &index](const std::string& params) {
        Register("getblock", [&index](const std::string& params) {
            auto hash = ParseHash(params);
            uint32_t height{0};
            if (!index.LookupBlock(hash, height)) throw std::runtime_error("unknown block");
            auto blk = ReadBlock(height);
            std::string hexBody = "";
            if (blk) {
                std::vector<uint8_t> raw;
                raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&blk->header), reinterpret_cast<uint8_t*>(&blk->header) + sizeof(BlockHeader));
                uint32_t txCount = static_cast<uint32_t>(blk->transactions.size());
                raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&txCount), reinterpret_cast<uint8_t*>(&txCount) + sizeof(txCount));
                for (const auto& tx : blk->transactions) {
                    auto ser = Serialize(tx);
                    uint32_t txSize = static_cast<uint32_t>(ser.size());
                    raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&txSize), reinterpret_cast<uint8_t*>(&txSize) + sizeof(txSize));
                    raw.insert(raw.end(), ser.begin(), ser.end());
                }
                hexBody = HexEncode(raw);
            }
            return std::string("{\"height\":") + std::to_string(height) + ",\"hex\":\"" + hexBody + "\"}";
        });

    Register("getblock", [this, &index](const std::string& params) {
        auto hash = ParseHash(params);
        uint32_t height{0};
        if (!index.LookupBlock(hash, height)) throw std::runtime_error("unknown block");
        auto blk = ReadBlock(height);
        std::string hexBody = "";
        if (blk) {
            std::vector<uint8_t> raw;
            raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&blk->header), reinterpret_cast<uint8_t*>(&blk->header) + sizeof(BlockHeader));
            uint32_t txCount = static_cast<uint32_t>(blk->transactions.size());
            raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&txCount), reinterpret_cast<uint8_t*>(&txCount) + sizeof(txCount));
            for (const auto& tx : blk->transactions) {
                auto ser = Serialize(tx);
                uint32_t txSize = static_cast<uint32_t>(ser.size());
                raw.insert(raw.end(), reinterpret_cast<uint8_t*>(&txSize), reinterpret_cast<uint8_t*>(&txSize) + sizeof(txSize));
                raw.insert(raw.end(), ser.begin(), ser.end());
            }
            hexBody = HexEncode(raw);
        }
        return std::string("{\"height\":") + std::to_string(height) + ",\"hex\":\"" + hexBody + "\"}";
    });
            return std::string("{\"accepted\":") + (ok ? "true" : "false") + "}";
        });

        Register("sendrawtransaction", [this](const std::string& params) {
            return GetHandler("sendtx")(params);
        });

        Register("gettransaction", [&index](const std::string& params) {
            auto hash = ParseHash(params);
            uint32_t height{0};
            bool found = index.Lookup(hash, height);
            return std::string("{\"found\":") + (found ? "true" : "false") + ",\"height\":" + std::to_string(height) + "}";
        });

        Register("estimatefee", [&pool](const std::string& params) {
            size_t percentile = 50;
            try { percentile = std::stoul(TrimQuotes(params)); } catch (...) {}
            return std::to_string(pool.EstimateFeeRate(percentile));
        });
    }

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

    Register("gettransaction", [&index](const std::string& params) {
        auto hash = ParseHash(params);
        uint32_t height{0};
        bool found = index.Lookup(hash, height);
        return std::string("{\"found\":") + (found ? "true" : "false") + ",\"height\":" + std::to_string(height) + "}";
    });

    Register("estimatefee", [&pool](const std::string& params) {
        size_t percentile = 50;
        try { percentile = std::stoul(TrimQuotes(params)); } catch (...) {}
        return std::to_string(pool.EstimateFeeRate(percentile));
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
    m_acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (!ec) HandleSession(std::move(socket));
        Accept();
    });
}

void RPCServer::HandleSession(boost::asio::ip::tcp::socket socket)
{
    auto buffer = std::make_shared<boost::beast::flat_buffer>();
    auto request = std::make_shared<http::request<http::string_body>>();
    http::async_read(socket, *buffer, *request, [this, buffer, request, s = std::move(socket)](boost::system::error_code ec, std::size_t) mutable {
        if (ec) return;
        auto response = Process(*request);
        http::async_write(s, response, [s = std::move(s)](boost::system::error_code, std::size_t) mutable {
            s.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
        });
    });
}

http::response<http::string_body> RPCServer::Process(const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{http::status::unauthorized, req.version()};
    res.set(http::field::content_type, "application/json");

    auto auth = req[http::field::authorization];
    if (!CheckAuth(auth)) {
        res.body() = R"({"error":"unauthorized"})";
        res.prepare_payload();
        return res;
    }

    try {
        const auto [method, params] = ParseJsonRpc(req.body());
        auto handler = GetHandler(method);
        if (!handler) throw std::runtime_error("method not found");
        std::string result = handler(params);
        std::string reply = std::string("{\"result\":") + result + ",\"error\":null}";
        res.result(http::status::ok);
        res.body() = reply;
    } catch (const std::exception& e) {
        res.result(http::status::bad_request);
        res.body() = std::string("{\"error\":\"") + e.what() + "\"}";
    }
    res.prepare_payload();
    return res;
}

bool RPCServer::CheckAuth(const std::string& header) const
{
    const std::string token = "Basic " + m_user + ":" + m_pass;
    return header == token;
}

RPCServer::Handler RPCServer::GetHandler(const std::string& name)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_handlers.find(name);
    if (it == m_handlers.end()) return nullptr;
    return it->second;
}

std::string RPCServer::HexEncode(const std::vector<uint8_t>& data)
{
    static const char* hexmap = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(hexmap[b >> 4]);
        out.push_back(hexmap[b & 0x0F]);
    }
    return out;
}

std::optional<Block> RPCServer::ReadBlock(uint32_t height)
{
    std::ifstream in(m_blockPath, std::ios::binary);
    if (!in.good()) return std::nullopt;
    uint32_t current = 0;
    while (in.peek() != EOF) {
        uint32_t size{0};
        auto start = in.tellg();
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        if (!in || size == 0) break;
        std::vector<uint8_t> data(size);
        in.read(reinterpret_cast<char*>(data.data()), size);
        if (!in) break;
        if (current == height) {
            Block block{};
            size_t offset = 0;
            if (size < sizeof(BlockHeader) + sizeof(uint32_t)) return std::nullopt;
            std::memcpy(&block.header, data.data(), sizeof(BlockHeader));
            offset += sizeof(BlockHeader);
            uint32_t txCount{0};
            std::memcpy(&txCount, data.data() + offset, sizeof(txCount));
            offset += sizeof(txCount);
            for (uint32_t i = 0; i < txCount; ++i) {
                if (offset + sizeof(uint32_t) > data.size()) return std::nullopt;
                uint32_t txSize{0};
                std::memcpy(&txSize, data.data() + offset, sizeof(txSize));
                offset += sizeof(txSize);
                if (offset + txSize > data.size()) return std::nullopt;
                std::vector<uint8_t> txdata(data.begin() + offset, data.begin() + offset + txSize);
                offset += txSize;
                block.transactions.push_back(DeserializeTransaction(txdata));
            }
            return block;
    static std::string HexEncode(const std::vector<uint8_t>& data)
    {
        static const char* hexmap = "0123456789abcdef";
        std::string out;
        out.reserve(data.size() * 2);
        for (auto b : data) {
            out.push_back(hexmap[b >> 4]);
            out.push_back(hexmap[b & 0x0F]);
        }
        return out;
    }

    std::optional<Block> ReadBlock(uint32_t height)
    {
        std::ifstream in(m_blockPath, std::ios::binary);
        if (!in.good()) return std::nullopt;
        // naive scan until height match
        uint32_t current = 0;
        while (in.peek() != EOF) {
            uint32_t size{0};
            auto start = in.tellg();
            in.read(reinterpret_cast<char*>(&size), sizeof(size));
            if (!in || size == 0) break;
            std::vector<uint8_t> data(size);
            in.read(reinterpret_cast<char*>(data.data()), size);
            if (!in) break;
            if (current == height) {
                Block block{};
                size_t offset = 0;
                if (size < sizeof(BlockHeader) + sizeof(uint32_t)) return std::nullopt;
                std::memcpy(&block.header, data.data(), sizeof(BlockHeader));
                offset += sizeof(BlockHeader);
                uint32_t txCount{0};
                std::memcpy(&txCount, data.data() + offset, sizeof(txCount));
                offset += sizeof(txCount);
                for (uint32_t i = 0; i < txCount; ++i) {
                    if (offset + sizeof(uint32_t) > data.size()) return std::nullopt;
                    uint32_t txSize{0};
                    std::memcpy(&txSize, data.data() + offset, sizeof(txSize));
                    offset += sizeof(txSize);
                    if (offset + txSize > data.size()) return std::nullopt;
                    std::vector<uint8_t> txdata(data.begin() + offset, data.begin() + offset + txSize);
                    offset += txSize;
                    block.transactions.push_back(DeserializeTransaction(txdata));
                }
                return block;
            }
            ++current;
            in.seekg(start + static_cast<std::streamoff>(sizeof(uint32_t) + size));
        }
        return std::nullopt;
    }

    static std::vector<uint8_t> ParseHex(const std::string& hex)
    {
        std::vector<uint8_t> out;
        if (hex.size() % 2) return out;
        out.reserve(hex.size() / 2);
        for (size_t i = 0; i < hex.size(); i += 2) {
            auto byte = std::stoul(hex.substr(i, 2), nullptr, 16);
            out.push_back(static_cast<uint8_t>(byte));
        }
        ++current;
        in.seekg(start + static_cast<std::streamoff>(sizeof(uint32_t) + size));
    }
    return std::nullopt;
}

std::vector<uint8_t> RPCServer::ParseHex(const std::string& hex)
{
    std::vector<uint8_t> out;
    if (hex.size() % 2) return out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        auto byte = std::stoul(hex.substr(i, 2), nullptr, 16);
        out.push_back(static_cast<uint8_t>(byte));
    }
    return out;
}

uint256 RPCServer::ParseHash(const std::string& params)
{
    auto hex = TrimQuotes(params);
    auto raw = ParseHex(hex);
    if (raw.size() != 32) throw std::runtime_error("bad hash");
    uint256 h{};
    std::copy(raw.begin(), raw.end(), h.begin());
    return h;
}

std::string RPCServer::TrimQuotes(std::string in)
{
    if (!in.empty() && in.front() == '"') in.erase(in.begin());
    if (!in.empty() && in.back() == '"') in.pop_back();
    return in;
}

std::pair<std::string, std::string> RPCServer::ParseJsonRpc(const std::string& body)
{
    std::regex methodRe("\"method\"\\s*:\\s*\"([^\"]*)\"");
    std::regex paramsRe("\"params\"\\s*:\\s*(.*)");
    std::smatch m;
    std::string method;
    std::string params = "null";
    if (std::regex_search(body, m, methodRe) && m.size() > 1) method = m[1];
    if (std::regex_search(body, m, paramsRe) && m.size() > 1) params = m[1];
    return {method, params};
}
    boost::asio::io_context& m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::string m_user;
    std::string m_pass;
    std::unordered_map<std::string, Handler> m_handlers;
    mutable std::mutex m_mutex;
    std::string m_blockPath{"mainnet/blocks.dat"};
};

} // namespace rpc
