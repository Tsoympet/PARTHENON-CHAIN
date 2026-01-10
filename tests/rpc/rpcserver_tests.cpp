#include <gtest/gtest.h>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <thread>
#include <filesystem>
#include <memory>
#include <sstream>

#include "../../layer2-services/rpc/rpcserver.h"
#include "../../layer2-services/policy/policy.h"
#include "../../layer2-services/mempool/mempool.h"
#include "../../layer2-services/index/txindex.h"
#include "../../layer2-services/wallet/wallet.h"
#include "../../sidechain/rpc/wasm_rpc.h"
#include "../../sidechain/wasm/runtime/engine.h"
#include "../../sidechain/state/state_store.h"

namespace http = boost::beast::http;

constexpr char kInvalidAuthHeader[] = "Basic invalid_base64";
constexpr uint8_t kWalletPrivSeed = 42;
constexpr uint8_t kWalletIdSeed = 99;
constexpr int kRateLimitRequests = 121;
constexpr int kMaxRetryAttempts = 3;
constexpr int kRetryDelayMs = 25;
constexpr int kServerStartupDelayMs = 50;

static http::response<http::string_body> RpcCallResponse(boost::asio::io_context& io, uint16_t port,
                                                         const std::string& body, bool include_auth = true,
                                                         const std::string& token = "",
                                                         const std::string& auth_override = "")
{
    for (int attempt = 0; attempt < kMaxRetryAttempts; ++attempt) {
        try {
            boost::asio::ip::tcp::resolver resolver(io);
            boost::beast::tcp_stream stream(io);
            auto const results = resolver.resolve("127.0.0.1", std::to_string(port));
            stream.connect(results);

            http::request<http::string_body> req{http::verb::post, "/", 11};
            req.set(http::field::host, "127.0.0.1");
            if (include_auth) {
                req.set(http::field::authorization, auth_override.empty() ? "Basic dXNlcjpwYXNz" : auth_override);
            }
            if (!token.empty()) {
                req.set("X-Auth-Token", token);
            }
            req.body() = body;
            req.prepare_payload();

            http::write(stream, req);
            boost::beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);
            return res;
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kRetryDelayMs));
        }
    }
    return {};
}

static std::string RpcCall(boost::asio::io_context& io, uint16_t port, const std::string& body,
                           bool include_auth = true, const std::string& token = "", const std::string& auth_override = "")
{
    return RpcCallResponse(io, port, body, include_auth, token, auth_override).body();
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

struct RpcTestHarness {
    uint16_t rpc_port;
    uint16_t p2p_port;
    boost::asio::io_context io;
    policy::FeePolicy policy;
    mempool::Mempool pool;
    wallet::KeyStore store;
    wallet::WalletBackend wallet;
    txindex::TxIndex index;
    net::P2PNode p2p;
    sidechain::wasm::ExecutionEngine engine;
    sidechain::state::StateStore state;
    sidechain::rpc::WasmRpcService wasm;
    std::unique_ptr<rpc::RPCServer> server;
    std::thread thread;

    explicit RpcTestHarness(uint16_t base_port)
        : rpc_port(base_port),
          p2p_port(static_cast<uint16_t>(base_port + 1)),
          policy(1, 100000, 100),
          pool(policy),
          wallet(store),
          p2p(io, p2p_port),
          wasm(engine, state)
    {
    }

    void FundWallet()
    {
        wallet::PrivKey priv{}; priv.fill(kWalletPrivSeed);
        wallet::KeyId id{}; id.fill(kWalletIdSeed);
        store.Import(id, priv);
        OutPoint op{}; op.hash.fill(1); op.index = 0;
        TxOut out; out.value = 5000; out.assetId = static_cast<uint8_t>(AssetId::DRACHMA); wallet.AddUTXO(op, out);
        index.AddBlock(op.hash, 0);
    }

    void Start(bool attach_core = true, bool attach_sidechain = true, bool prefund_wallet = true)
    {
        server = std::make_unique<rpc::RPCServer>(io, "user", "pass", rpc_port);
        if (attach_core) {
            if (prefund_wallet) {
                FundWallet();
            }
            server->AttachCoreHandlers(pool, wallet, index, p2p);
        }
        if (attach_sidechain) {
            server->AttachSidechainHandlers(wasm);
        }
        server->Start();
        thread = std::thread([this]() { io.run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(kServerStartupDelayMs));
    }

    void Stop()
    {
        io.stop();
        if (thread.joinable()) {
            thread.join();
        }
    }

    ~RpcTestHarness() { Stop(); }
};

TEST(TxIndex, UsesCacheWhenDbAbsent)
{
    txindex::TxIndex index;
    uint256 h{};
    h.fill(0x01);
    uint32_t heightOut = 0;
    EXPECT_FALSE(index.Lookup(h, heightOut));

    index.AddBlock(h, 5);
    EXPECT_TRUE(index.LookupBlock(h, heightOut));
    EXPECT_EQ(heightOut, 5u);

    // Persisted index should survive reopen and ignore empty DB entries.
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "txindex_disk";
    std::filesystem::remove_all(tmp);
    uint256 tx{};
    tx.fill(0x02);
    {
        txindex::TxIndex disk;
        disk.Open(tmp.string());
        uint32_t missing{0};
        EXPECT_FALSE(disk.Lookup(tx, missing));
        disk.Add(tx, 42);
        EXPECT_TRUE(disk.Lookup(tx, heightOut));
        EXPECT_EQ(heightOut, 42u);
    }
    txindex::TxIndex reloaded;
    reloaded.Open(tmp.string());
    EXPECT_TRUE(reloaded.Lookup(tx, heightOut));
    EXPECT_EQ(heightOut, 42u);
}

TEST(TxIndex, ReopensEmptyAndTracksNewBlocks)
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path() / "txindex_reindex";
    std::filesystem::remove_all(tmp);

    const uint8_t missingSeed = 0x0a;
    const uint8_t blockSeed = 0x0b;

    {
        txindex::TxIndex disk;
        disk.Open(tmp.string());
        uint256 missing{};
        missing.fill(missingSeed);
        uint32_t heightOut{0};
        EXPECT_FALSE(disk.Lookup(missing, heightOut));
        disk.Add(missing, 9);
        EXPECT_TRUE(disk.Lookup(missing, heightOut));
        EXPECT_EQ(heightOut, 9u);
    }

    txindex::TxIndex reopened;
    reopened.Open(tmp.string());
    uint32_t out{0};
    uint256 missing{};
    missing.fill(missingSeed);
    EXPECT_TRUE(reopened.Lookup(missing, out));
    EXPECT_EQ(out, 9u);
    EXPECT_EQ(reopened.BlockCount(), 0u);

    uint256 block{};
    block.fill(blockSeed);
    EXPECT_FALSE(reopened.LookupBlock(block, out));
    reopened.AddBlock(block, 3);
    EXPECT_EQ(reopened.BlockCount(), 1u);
    EXPECT_TRUE(reopened.LookupBlock(block, out));
    EXPECT_EQ(out, 3u);
}

TEST(RPC, EndpointsRespond)
{
    RpcTestHarness env(19600);
    env.Start(true, true);

    std::string balance = RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":null}");
    EXPECT_NE(balance.find("\"DRM\""), std::string::npos);
    EXPECT_NE(balance.find("5000"), std::string::npos);
    EXPECT_NE(balance.find("\"TLN\""), std::string::npos);
    EXPECT_NE(balance.find("\"OBL\""), std::string::npos);

    std::string height = RpcCall(env.io, env.rpc_port, "{\"method\":\"getblockcount\",\"params\":null}");
    EXPECT_NE(height.find("1"), std::string::npos);

    std::string error = RpcCall(env.io, env.rpc_port, "{\"method\":\"unknown\",\"params\":null}");
    EXPECT_EQ(error, "{\"error\":\"unknown method\"}");

    std::string staking = RpcCall(env.io, env.rpc_port, "{\"method\":\"getstakinginfo\",\"params\":null}");
    EXPECT_NE(staking.find("\"DRM\""), std::string::npos);
    EXPECT_NE(staking.find("\"OBL\""), std::string::npos);
    // All assets are now PoW-only, so posAllowed should be false for all
    EXPECT_EQ(staking.find("\"posAllowed\":true"), std::string::npos);
    EXPECT_NE(staking.find("\"posAllowed\":false"), std::string::npos);

    std::string numericParams = RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":123}");
    EXPECT_EQ(numericParams, "{\"result\":null}");

    std::string unauthorized = RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":null}", false);
    EXPECT_EQ(unauthorized, "{\"error\":\"auth required\"}");

    std::string badCredentials =
        RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":null}", true, "", "Basic ZmFrZTp3cm9uZw==");
    EXPECT_EQ(badCredentials, "{\"error\":\"auth required\"}");

    std::string tokenAuthorized =
        RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":null}", false, "drachma-token");
    EXPECT_NE(tokenAuthorized.find("\"DRM\""), std::string::npos);

    std::string invalidBalance =
        RpcCall(env.io, env.rpc_port, R"({"method":"getbalance","params":"\"invalid-asset\""})");
    EXPECT_NE(invalidBalance.find("null"), std::string::npos);

    std::string malformedJson = RpcCall(env.io, env.rpc_port, "not-json");
    EXPECT_NE(malformedJson.find("error"), std::string::npos);

    // Sidechain RPCs (WASM-only, asset/domain bound)
    std::string codeHex = ConstThenReturnHex(1);
    std::string deploy = RpcCall(env.io, env.rpc_port, std::string("{\"method\":\"deploy_contract\",\"params\":\"module=test.mod;asset=1;gas=100;code=") + codeHex + "\"}");
    EXPECT_NE(deploy.find("\"success\":true"), std::string::npos);

    std::string call = RpcCall(env.io, env.rpc_port, std::string("{\"method\":\"call_contract\",\"params\":\"module=test.mod;asset=1;gas=100;code=") + codeHex + "\"}");
    EXPECT_NE(call.find("\"success\":true"), std::string::npos);

    std::string wrongContractAsset = RpcCall(env.io, env.rpc_port, std::string("{\"method\":\"call_contract\",\"params\":\"module=test.mod;asset=2;gas=50;code=") + codeHex + "\"}");
    EXPECT_NE(wrongContractAsset.find("asset/domain violation"), std::string::npos);

    std::string mint = RpcCall(env.io, env.rpc_port, "{\"method\":\"mint_nft\",\"params\":\"token=token-1;owner=alice;meta=hash;asset=0;gas=50\"}");
    EXPECT_NE(mint.find("\"success\":true"), std::string::npos);

    std::string transfer = RpcCall(env.io, env.rpc_port, "{\"method\":\"transfer_nft\",\"params\":\"token=token-1;from=alice;to=bob;asset=0;gas=50\"}");
    EXPECT_NE(transfer.find("\"success\":true"), std::string::npos);

    std::string badMint = RpcCall(env.io, env.rpc_port, "{\"method\":\"mint_nft\",\"params\":\"token=token-2;owner=alice;meta=;asset=0;gas=50\"}");
    EXPECT_NE(badMint.find("invalid canon reference"), std::string::npos);

    std::string tlnListing = RpcCall(
        env.io, env.rpc_port, "{\"method\":\"list_nft\",\"params\":\"token=token-1;seller=bob;asset=0;price=1;gas=50\"}");
    EXPECT_NE(tlnListing.find("payment must be DRM or OBL"), std::string::npos);

    std::string dappCode = ConstThenReturnHex(7);
    std::string dapp = RpcCall(env.io, env.rpc_port, std::string("{\"method\":\"call_dapp\",\"params\":\"app=dapp.mod;asset=2;gas=100;code=") + dappCode + "\"}");
    EXPECT_NE(dapp.find("\"success\":true"), std::string::npos);
    EXPECT_NE(dapp.find("07000000"), std::string::npos); // output hex

    std::string wrongDappAsset = RpcCall(env.io, env.rpc_port, std::string("{\"method\":\"call_dapp\",\"params\":\"app=dapp.mod;asset=1;gas=25;code=") + dappCode + "\"}");
    EXPECT_NE(wrongDappAsset.find("asset/domain violation"), std::string::npos);

    std::string malformed = RpcCall(env.io, env.rpc_port, "{\"method\":\"sendtx\",\"params\":\"zz\"}");
    EXPECT_NE(malformed.find("error"), std::string::npos);

    std::string badSend = RpcCall(env.io, env.rpc_port, "{\"method\":\"sendtx\",\"params\":\"00\"}");
    EXPECT_NE(badSend.find("deserialize uint32 overflow"), std::string::npos);
}

TEST(RPCFailurePaths, AuthAndRateLimitEnforced)
{
    RpcTestHarness env(19610);
    env.Start(true, false);

    auto invalidAuth = RpcCallResponse(env.io, env.rpc_port,
        "{\"method\":\"getblockcount\",\"params\":null}", true, "", kInvalidAuthHeader);
    EXPECT_EQ(invalidAuth.result(), http::status::unauthorized);
    EXPECT_EQ(invalidAuth.body(), "{\"error\":\"auth required\"}");

    http::response<http::string_body> last;
    for (int i = 0; i < kRateLimitRequests; ++i) {
        last = RpcCallResponse(env.io, env.rpc_port, "{\"method\":\"getblockcount\",\"params\":null}");
    }
    EXPECT_EQ(last.result(), http::status::too_many_requests);
    EXPECT_NE(last.body().find("rate limited"), std::string::npos);
}

TEST(RPCFailurePaths, UnknownMethodsWhenSubsystemsDisabled)
{
    RpcTestHarness uninitialized(19620);
    uninitialized.Start(false, false, false);
    auto walletRequired = RpcCallResponse(uninitialized.io, uninitialized.rpc_port,
        "{\"method\":\"getbalance\",\"params\":null}");
    EXPECT_EQ(walletRequired.result(), http::status::bad_request);
    EXPECT_EQ(walletRequired.body(), "{\"error\":\"unknown method\"}");

    RpcTestHarness coreOnly(19622);
    coreOnly.Start(true, false);
    auto nftDisabled = RpcCallResponse(coreOnly.io, coreOnly.rpc_port,
        "{\"method\":\"mint_nft\",\"params\":\"token=na;owner=alice;meta=hash\"}");
    EXPECT_EQ(nftDisabled.result(), http::status::bad_request);
    EXPECT_EQ(nftDisabled.body(), "{\"error\":\"unknown method\"}");
}

TEST(RPCAssetValidation, RejectsTlnWhereForbiddenAndMixedAssets)
{
    RpcTestHarness env(19630);
    env.Start(true, true);

    std::string codeHex = ConstThenReturnHex(0);
    auto tlnCall = RpcCallResponse(env.io, env.rpc_port,
        std::string("{\"method\":\"call_contract\",\"params\":\"module=guard;asset=0;gas=10;code=") + codeHex + "\"}");
    EXPECT_EQ(tlnCall.result(), http::status::ok);
    EXPECT_NE(tlnCall.body().find("asset/domain violation"), std::string::npos);

    RpcCall(env.io, env.rpc_port, "{\"method\":\"mint_nft\",\"params\":\"token=asset-mix;owner=alice;meta=hash;canon=hash;asset=0;gas=50\"}");
    RpcCall(env.io, env.rpc_port, "{\"method\":\"list_nft\",\"params\":\"token=asset-mix;seller=alice;asset=1;price=5;gas=50\"}");
    auto mixedAssets = RpcCall(env.io, env.rpc_port,
        "{\"method\":\"place_nft_bid\",\"params\":\"token=asset-mix;bidder=bob;asset=2;price=5;gas=50\"}");
    EXPECT_NE(mixedAssets.find("asset mismatch"), std::string::npos);

    auto invalidAssetId =
        RpcCall(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":5}");
    EXPECT_EQ(invalidAssetId, "{\"result\":null}");
}

TEST(RPCFailurePaths, ExceptionsPropagateWithoutStateMutation)
{
    RpcTestHarness env(19640);
    env.Start(true, true);

    auto missingParams = RpcCallResponse(env.io, env.rpc_port, "{\"method\":\"sendtx\"}");
    EXPECT_EQ(missingParams.result(), http::status::internal_server_error);
    EXPECT_NE(missingParams.body().find("deserialize"), std::string::npos);
    EXPECT_TRUE(env.pool.Snapshot().empty());

    auto badCode = RpcCallResponse(env.io, env.rpc_port,
        "{\"method\":\"call_contract\",\"params\":\"module=bad;asset=1;gas=1;code=zz\"}");
    EXPECT_EQ(badCode.result(), http::status::internal_server_error);
    EXPECT_NE(badCode.body().find("error"), std::string::npos);

    auto stillAlive = RpcCall(env.io, env.rpc_port, "{\"method\":\"getblockcount\",\"params\":null}");
    EXPECT_NE(stillAlive.find("1"), std::string::npos);

    auto failedMint = RpcCall(env.io, env.rpc_port,
        "{\"method\":\"mint_nft\",\"params\":\"token=rollback;owner=alice;meta=;asset=0;gas=50\"}");
    EXPECT_NE(failedMint.find("invalid canon reference"), std::string::npos);

    auto missingAfterFailure = RpcCall(env.io, env.rpc_port,
        "{\"method\":\"transfer_nft\",\"params\":\"token=rollback;from=alice;to=bob;asset=0;gas=50\"}");
    EXPECT_NE(missingAfterFailure.find("token missing"), std::string::npos);
}

TEST(RPCFailurePaths, ArgumentErrorsReturnExplicitCodes)
{
    RpcTestHarness env(19650);
    env.Start(true, true);

    auto malformed = RpcCallResponse(env.io, env.rpc_port, "%%%");
    EXPECT_EQ(malformed.result(), http::status::bad_request);
    EXPECT_EQ(malformed.body(), "{\"error\":\"unknown method\"}");

    auto wrongType = RpcCallResponse(env.io, env.rpc_port,
        "{\"method\":\"call_contract\",\"params\":\"module=bad;asset=abc;gas=1;code=00\"}");
    EXPECT_EQ(wrongType.result(), http::status::internal_server_error);
    EXPECT_NE(wrongType.body().find("\"error\":\""), std::string::npos);

    auto unauthorized = RpcCallResponse(env.io, env.rpc_port,
        "{\"method\":\"getbalance\",\"params\":null}", false);
    EXPECT_EQ(unauthorized.result(), http::status::unauthorized);
    EXPECT_EQ(unauthorized.body(), "{\"error\":\"auth required\"}");

    EXPECT_TRUE(env.pool.Snapshot().empty());
}

TEST(RPCFailurePaths, WalletDisabledAndMalformedParameters)
{
    RpcTestHarness env(19660);
    // Start(attach_core=false, attach_sidechain=true, prefund_wallet=false): wallet/mempool disabled, sidechain only
    env.Start(false, true, false);

    auto walletCall = RpcCallResponse(env.io, env.rpc_port, "{\"method\":\"getbalance\",\"params\":null}");
    EXPECT_EQ(walletCall.result(), http::status::bad_request);
    EXPECT_EQ(walletCall.body(), "{\"error\":\"unknown method\"}");

    std::string codeHex = ConstThenReturnHex(3);
    auto tlnDeploy = RpcCallResponse(env.io, env.rpc_port,
        std::string("{\"method\":\"deploy_contract\",\"params\":\"module=guard;asset=0;gas=10;code=") + codeHex + "\"}");
    EXPECT_EQ(tlnDeploy.result(), http::status::ok);
    EXPECT_NE(tlnDeploy.body().find("asset/domain violation"), std::string::npos);

    auto malformedParams = RpcCallResponse(env.io, env.rpc_port,
        "{\"method\":\"deploy_contract\",\"params\":\"module=guard;asset=1;gas=invalid;code=00\"}");
    EXPECT_EQ(malformedParams.result(), http::status::internal_server_error);
    EXPECT_NE(malformedParams.body().find("error"), std::string::npos);
}

TEST(RPCJsonFormatting, EscapesErrorStrings)
{
    sidechain::wasm::ExecutionResult result;
    result.success = false;
    result.gas_used = 0;
    result.state_writes = 0;
    result.error = "bad \"quote\"\nline";

    const auto json = rpc::FormatExecResult(result);
    std::stringstream ss(json);
    boost::property_tree::ptree pt;
    EXPECT_NO_THROW(boost::property_tree::read_json(ss, pt));
    EXPECT_EQ(pt.get<std::string>("error"), result.error);
}
