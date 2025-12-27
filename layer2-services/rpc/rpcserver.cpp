#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <algorithm>
#include <functional>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../mempool/mempool.h"
#include "../net/p2p.h"
#include "../wallet/wallet.h"
#include "../index/txindex.h"
#include "../../layer1-core/tx/transaction.h"

namespace http = boost::beast::http;

namespace rpc {

class RPCServer {
public:
    using Handler = std::function<std::string(const std::string&)>;

    RPCServer(boost::asio::io_context& io, const std::string& user, const std::string& pass, uint16_t port)
        : m_io(io), m_acceptor(io, {boost::asio::ip::tcp::v4(), port}), m_user(user), m_pass(pass) {}

    void AttachCoreHandlers(mempool::Mempool& pool, wallet::WalletBackend& wallet, txindex::TxIndex& index, net::P2PNode& p2p)
    {
        Register("getbalance", [&wallet](const std::string&) {
            return std::to_string(wallet.GetBalance());
        });

        Register("getblock", [&index](const std::string& params) {
            auto hash = ParseHash(params);
            uint32_t height{0};
            if (!index.LookupBlock(hash, height)) throw std::runtime_error("unknown block");
            return std::string("{\"height\":") + std::to_string(height) + "}";
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
    }

    void Register(const std::string& method, Handler handler)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_handlers[method] = std::move(handler);
    }

    void Start()
    {
        Accept();
    }

    void Stop()
    {
        boost::system::error_code ec;
        m_acceptor.close(ec);
    }

private:
    void Accept()
    {
        m_acceptor.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) HandleSession(std::move(socket));
            Accept();
        });
    }

    void HandleSession(boost::asio::ip::tcp::socket socket)
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

    http::response<http::string_body> Process(const http::request<http::string_body>& req)
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

    bool CheckAuth(const std::string& header) const
    {
        const std::string token = "Basic " + m_user + ":" + m_pass;
        return header == token;
    }

    Handler GetHandler(const std::string& name)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        auto it = m_handlers.find(name);
        if (it == m_handlers.end()) return nullptr;
        return it->second;
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
        return out;
    }

    static uint256 ParseHash(const std::string& params)
    {
        auto hex = TrimQuotes(params);
        auto raw = ParseHex(hex);
        if (raw.size() != 32) throw std::runtime_error("bad hash");
        uint256 h{};
        std::copy(raw.begin(), raw.end(), h.begin());
        return h;
    }

    static std::string TrimQuotes(std::string in)
    {
        if (!in.empty() && in.front() == '"') in.erase(in.begin());
        if (!in.empty() && in.back() == '"') in.pop_back();
        return in;
    }

    std::pair<std::string, std::string> ParseJsonRpc(const std::string& body)
    {
        // Minimal JSON-RPC parsing: {"method":"...","params":...}
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
};

} // namespace rpc
