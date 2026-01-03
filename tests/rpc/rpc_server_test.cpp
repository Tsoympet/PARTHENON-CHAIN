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
#include "../../sidechain/rpc/wasm_rpc.h"
#include "../../sidechain/wasm/runtime/engine.h"
#include "../../sidechain/state/state_store.h"

namespace http = boost::beast::http;

static std::string RpcCall(boost::asio::io_context& io, uint16_t port, const std::string& body,
                           bool include_auth = true)
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
            if (include_auth) {
                req.set(http::field::authorization, "Basic dXNlcjpwYXNz");
            }
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

static std::string Hex(const std::vector<uint8_t>& bytes)
{
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (auto b : bytes) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0xf]);
    }
    return out;
}

static std::string ConstThenReturnHex(int32_t imm)
{
    // OpCode::ConstI32 followed by OpCode::ReturnTop; each opcode reserves a 4-byte immediate slot.
    std::vector<uint8_t> code;
    auto append_le32 = [&](int32_t v) {
        code.push_back(static_cast<uint8_t>(v & 0xff));
        code.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
        code.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
        code.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
    };
    code.push_back(static_cast<uint8_t>(sidechain::wasm::OpCode::ConstI32));
    append_le32(imm);
    code.push_back(static_cast<uint8_t>(sidechain::wasm::OpCode::ReturnTop));
    append_le32(0); // unused immediate slot for ReturnTop
    return Hex(code);
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
    sidechain::wasm::ExecutionEngine engine;
    sidechain::state::StateStore state;
    sidechain::rpc::WasmRpcService wasm(engine, state);
    server.AttachCoreHandlers(pool, wallet, index, p2p);
    server.AttachSidechainHandlers(wasm);
    server.Start();

    std::thread t([&io]() { io.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string balance = RpcCall(io, 19600, "{\"method\":\"getbalance\",\"params\":null}");
    EXPECT_NE(balance.find("\"DRM\""), std::string::npos);
    EXPECT_NE(balance.find("5000"), std::string::npos);
    EXPECT_NE(balance.find("\"TLN\""), std::string::npos);
    EXPECT_NE(balance.find("\"OBL\""), std::string::npos);

    std::string height = RpcCall(io, 19600, "{\"method\":\"getblockcount\",\"params\":null}");
    EXPECT_NE(height.find("1"), std::string::npos);

    std::string error = RpcCall(io, 19600, "{\"method\":\"unknown\",\"params\":null}");
    EXPECT_NE(error.find("error"), std::string::npos);

    std::string staking = RpcCall(io, 19600, "{\"method\":\"getstakinginfo\",\"params\":null}");
    EXPECT_NE(staking.find("\"DRM\""), std::string::npos);
    EXPECT_NE(staking.find("\"OBL\""), std::string::npos);
    EXPECT_NE(staking.find("\"posAllowed\":true"), std::string::npos);

    std::string unauthorized = RpcCall(io, 19600, "{\"method\":\"getbalance\",\"params\":null}", false);
    EXPECT_NE(unauthorized.find("auth required"), std::string::npos);

    std::string invalidBalance =
        RpcCall(io, 19600, R"({"method":"getbalance","params":"\"invalid-asset\""})");
    EXPECT_NE(invalidBalance.find("null"), std::string::npos);

    // Sidechain RPCs (WASM-only, asset/domain bound)
    std::string codeHex = ConstThenReturnHex(1);
    std::string deploy = RpcCall(io, 19600, std::string("{\"method\":\"deploy_contract\",\"params\":\"module=test.mod;asset=1;gas=100;code=") + codeHex + "\"}");
    EXPECT_NE(deploy.find("\"success\":true"), std::string::npos);

    std::string call = RpcCall(io, 19600, std::string("{\"method\":\"call_contract\",\"params\":\"module=test.mod;asset=1;gas=100;code=") + codeHex + "\"}");
    EXPECT_NE(call.find("\"success\":true"), std::string::npos);

    std::string wrongContractAsset = RpcCall(io, 19600, std::string("{\"method\":\"call_contract\",\"params\":\"module=test.mod;asset=2;gas=50;code=") + codeHex + "\"}");
    EXPECT_NE(wrongContractAsset.find("asset/domain violation"), std::string::npos);

    std::string mint = RpcCall(io, 19600, "{\"method\":\"mint_nft\",\"params\":\"token=token-1;owner=alice;meta=hash;asset=0;gas=50\"}");
    EXPECT_NE(mint.find("\"success\":true"), std::string::npos);

    std::string transfer = RpcCall(io, 19600, "{\"method\":\"transfer_nft\",\"params\":\"token=token-1;from=alice;to=bob;asset=0;gas=50\"}");
    EXPECT_NE(transfer.find("\"success\":true"), std::string::npos);

    std::string badMint = RpcCall(io, 19600, "{\"method\":\"mint_nft\",\"params\":\"token=token-2;owner=alice;meta=;asset=0;gas=50\"}");
    EXPECT_NE(badMint.find("invalid canon reference"), std::string::npos);

    std::string dappCode = ConstThenReturnHex(7);
    std::string dapp = RpcCall(io, 19600, std::string("{\"method\":\"call_dapp\",\"params\":\"app=dapp.mod;asset=2;gas=100;code=") + dappCode + "\"}");
    EXPECT_NE(dapp.find("\"success\":true"), std::string::npos);
    EXPECT_NE(dapp.find("07000000"), std::string::npos); // output hex

    std::string wrongDappAsset = RpcCall(io, 19600, std::string("{\"method\":\"call_dapp\",\"params\":\"app=dapp.mod;asset=1;gas=25;code=") + dappCode + "\"}");
    EXPECT_NE(wrongDappAsset.find("asset/domain violation"), std::string::npos);

    io.stop();
    t.join();
}
