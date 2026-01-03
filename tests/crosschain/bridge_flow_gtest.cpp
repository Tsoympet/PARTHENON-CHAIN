#include <gtest/gtest.h>
#include <filesystem>
#include <openssl/sha.h>
#include <chrono>
#include <thread>
#include "../../layer2-services/crosschain/bridge/bridge_manager.h"
#include "../../layer2-services/crosschain/relayer/relayer.h"
#include "../../layer2-services/net/p2p.h"

using namespace crosschain;

namespace {
std::array<uint8_t, 32> Hash(const std::vector<uint8_t>& data)
{
    std::array<uint8_t, 32> h{};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(h.data(), &ctx);
    return h;
}
}

TEST(BridgeFlow, InitiateClaimRefund)
{
    auto tmp = std::filesystem::temp_directory_path() / "bridge_flow";
    BridgeManager mgr(tmp.string());

    ChainConfig cfg{};
    mgr.RegisterChain("bitcoin", cfg);

    std::vector<uint8_t> secret = {1, 2, 3, 4};
    auto secretHash = Hash(secret);
    std::array<uint8_t, 32> priv{};
    priv[0] = 1;
    std::vector<uint8_t> sig;
    auto lock = mgr.InitiateOutboundLock("bitcoin", "tx1", "addr", 50, secretHash, 100, priv, sig);
    EXPECT_FALSE(sig.empty());

    std::vector<uint8_t> claimSig;
    EXPECT_TRUE(mgr.Claim(lock.id, secret, 10, claimSig));
    EXPECT_FALSE(claimSig.empty());

    EXPECT_FALSE(mgr.Refund(lock.id, 50));
    EXPECT_TRUE(mgr.Refund(lock.id, 150));
}

TEST(BridgeFlow, DetectsInboundLock)
{
    auto tmp = std::filesystem::temp_directory_path() / "bridge_flow_inbound";
    BridgeManager mgr(tmp.string());
    ChainConfig cfg{};
    mgr.RegisterChain("litecoin", cfg);

    HeaderProof proof{};
    proof.height = 1;
    proof.header.fill(0);

    BridgeLock observed{};
    observed.chain = "litecoin";
    observed.txid = "lock";
    observed.destination = "drachma";
    observed.amount = 100;
    observed.timeoutHeight = 50;

    EXPECT_TRUE(mgr.DetectInboundLock("litecoin", {proof}, observed));
    auto pending = mgr.PendingFor("drachma");
    EXPECT_FALSE(pending.empty());
}

TEST(RelayerFlow, HandlesEmptyEndpoint)
{
    auto tmp = std::filesystem::temp_directory_path() / "relayer_tmp";
    BridgeManager mgr(tmp.string());
    boost::asio::io_context io;
    net::P2PNode p2p(io, 0);

    ChainConfig cfg{};
    cfg.rpcEndpoint = "";
    crosschain::Relayer rel(mgr, p2p, io);
    rel.AddWatchedChain("empty", cfg);
    rel.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    rel.Stop();
    EXPECT_EQ(rel.Metrics().detected.load(), 0u);
}
