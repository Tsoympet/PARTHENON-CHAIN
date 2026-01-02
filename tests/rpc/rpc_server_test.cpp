#include <gtest/gtest.h>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <chrono>
#include <thread>
#include "../../layer2-services/rpc/rpcserver.h"
#include "../../layer2-services/policy/policy.h"
#include "../../layer2-services/mempool/mempool.h"
#include "../../layer2-services/index/txindex.h"
#include "../../layer2-services/wallet/wallet.h"

namespace http = boost::beast::http;

static std::string RpcCall(boost::asio::io_context& io, uint16_t port, const std::string& body)
{
    boost::asio::io_context clientIo;
    for (int attempt = 0; attempt < 3; ++attempt) {
        try {
            boost::asio::ip::tcp::resolver resolver(clientIo);
            boost::beast::tcp_stream stream(clientIo);
            auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
            stream.connect(results);

            http::request<http::string_body> req{http::verb::post, "/", 11};
            req.set(http::field::host, "127.0.0.1");
            req.set(http::field::authorization, "Basic dXNlcjpwYXNz");
            req.body() = body;
            req.prepare_payload();

            http::write(stream, req);
            boost::beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            return res.body();
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }
    return "";
}

TEST(RPC, EndpointsRespond)
{
    boost::asio::io_context io;
    policy::FeePolicy policy(1, 100000, 100);
    mempool::Mempool pool(policy);

    wallet::KeyStore store;
    wallet::PrivKey priv{}; priv.fill(42);
    wallet::KeyId id{}; id.fill(99);
    store.Import(id, priv);
    wallet::WalletBackend wallet(store);
    OutPoint op{}; op.hash.fill(1); op.index = 0;
    TxOut out; out.value = 5000; out.assetId = static_cast<uint8_t>(AssetId::DRACHMA); wallet.AddUTXO(op, out);

    txindex::TxIndex index;
    index.AddBlock(op.hash, 0);

    net::P2PNode p2p(io, 19601);

    rpc::RPCServer server(io, "user", "pass", 19600);
    server.AttachCoreHandlers(pool, wallet, index, p2p);
    server.Start();

    std::thread t([&io]() { io.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string balance = RpcCall(io, 19600, "{\"method\":\"getbalance\",\"params\":null}");
    EXPECT_NE(balance.find("asset1"), std::string::npos);
    EXPECT_NE(balance.find("5000"), std::string::npos);

    std::string height = RpcCall(io, 19600, "{\"method\":\"getblockcount\",\"params\":null}");
    EXPECT_NE(height.find("1"), std::string::npos);

    std::string error = RpcCall(io, 19600, "{\"method\":\"unknown\",\"params\":null}");
    EXPECT_NE(error.find("error"), std::string::npos);

    io.stop();
    t.join();
}
