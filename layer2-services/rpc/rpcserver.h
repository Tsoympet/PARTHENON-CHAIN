#pragma once

#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../mempool/mempool.h"
#include "../net/p2p.h"
#include "../wallet/wallet.h"
#include "../index/txindex.h"
#include "../../layer1-core/block/block.h"
#include "../../layer1-core/tx/transaction.h"

namespace rpc {

class RPCServer {
public:
    using Handler = std::function<std::string(const std::string&)>;

    RPCServer(boost::asio::io_context& io, const std::string& user, const std::string& pass, uint16_t port);

    void SetBlockStorePath(std::string path);

    void AttachCoreHandlers(mempool::Mempool& pool, wallet::WalletBackend& wallet, txindex::TxIndex& index, net::P2PNode& p2p);

    void Register(const std::string& method, Handler handler);

    void Start();
    void Stop();

private:
    void Accept();
    void HandleSession(boost::asio::ip::tcp::socket socket);
    boost::beast::http::response<boost::beast::http::string_body> Process(const boost::beast::http::request<boost::beast::http::string_body>& req);
    bool CheckAuth(const std::string& header) const;
    Handler GetHandler(const std::string& name);
    static std::string HexEncode(const std::vector<uint8_t>& data);
    std::optional<Block> ReadBlock(uint32_t height);
    static std::vector<uint8_t> ParseHex(const std::string& hex);
    static uint256 ParseHash(const std::string& params);
    static std::string TrimQuotes(std::string in);
    std::pair<std::string, std::string> ParseJsonRpc(const std::string& body);

    boost::asio::io_context& m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::string m_user;
    std::string m_pass;
    std::unordered_map<std::string, Handler> m_handlers;
    mutable std::mutex m_mutex;
    std::string m_blockPath{"mainnet/blocks.dat"};
};

} // namespace rpc

