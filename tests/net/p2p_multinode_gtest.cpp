#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <optional>
#include <thread>
#include "../../layer2-services/net/p2p.h"

using namespace std::chrono_literals;

namespace {

void RunIo(boost::asio::io_context& io, std::atomic<bool>& stopFlag)
{
    while (!stopFlag.load()) {
        io.run_for(20ms);
        io.restart();
    }
}

} // namespace

TEST(P2P, MultinodeBroadcastAndInventory)
{
    boost::asio::io_context ioA;
    boost::asio::io_context ioB;
    boost::asio::io_context ioC;

    net::P2PNode nodeA(ioA, 0);
    net::P2PNode nodeB(ioB, 0);
    net::P2PNode nodeC(ioC, 0);

    nodeA.AddPeerAddress("127.0.0.1:" + std::to_string(nodeB.ListenPort()));
    nodeA.AddPeerAddress("127.0.0.1:" + std::to_string(nodeC.ListenPort()));
    nodeB.AddPeerAddress("127.0.0.1:" + std::to_string(nodeA.ListenPort()));
    nodeC.AddPeerAddress("127.0.0.1:" + std::to_string(nodeA.ListenPort()));

    std::atomic<bool> stop{false};
    std::thread tA(RunIo, std::ref(ioA), std::ref(stop));
    std::thread tB(RunIo, std::ref(ioB), std::ref(stop));
    std::thread tC(RunIo, std::ref(ioC), std::ref(stop));

    std::atomic<int> receivedHello{0};
    nodeB.RegisterHandler("hello", [&receivedHello](const net::PeerInfo&, const net::Message& msg) {
        if (!msg.payload.empty() && msg.payload[0] == 0x01) {
            receivedHello.fetch_add(1, std::memory_order_relaxed);
        }
    });
    nodeC.RegisterHandler("hello", [&receivedHello](const net::PeerInfo&, const net::Message& msg) {
        if (!msg.payload.empty() && msg.payload[0] == 0x01) {
            receivedHello.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::vector<uint8_t> offeredPayload{0xAB, 0xCD};
    std::atomic<int> receivedTx{0};
    uint256 advertised{};
    advertised[0] = 0xEE;
    nodeB.SetTxProvider([offeredPayload](const uint256& h) -> std::optional<std::vector<uint8_t>> {
        if (h[0] == 0xEE) return offeredPayload;
        return std::nullopt;
    });
    nodeC.RegisterHandler("tx", [&receivedTx, offeredPayload](const net::PeerInfo&, const net::Message& msg) {
        if (msg.payload == offeredPayload) receivedTx.fetch_add(1, std::memory_order_relaxed);
    });

    nodeA.Start();
    nodeB.Start();
    nodeC.Start();

    auto waitForPeers = [](net::P2PNode& n, size_t expected) {
        for (int i = 0; i < 200; ++i) {
            if (n.Peers().size() >= expected) return true;
            std::this_thread::sleep_for(10ms);
        }
        return false;
    };

    ASSERT_TRUE(waitForPeers(nodeA, 2));
    ASSERT_TRUE(waitForPeers(nodeB, 1));
    ASSERT_TRUE(waitForPeers(nodeC, 1));

    nodeA.Broadcast(net::Message{"hello", {0x01}});
    std::this_thread::sleep_for(200ms);
    EXPECT_GE(receivedHello.load(), 2);

    nodeB.AnnounceInventory({advertised});
    std::this_thread::sleep_for(300ms);
    EXPECT_GE(receivedTx.load(), 1);

    stop = true;
    tA.join();
    tB.join();
    tC.join();

    nodeA.Stop();
    nodeB.Stop();
    nodeC.Stop();
}
