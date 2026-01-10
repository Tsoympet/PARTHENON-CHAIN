#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
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

TEST(P2P, SeedHostnameDoesNotReconnect)
{
    boost::asio::io_context ioA;
    boost::asio::io_context ioB;

    net::P2PNode nodeA(ioA, 0);
    net::P2PNode nodeB(ioB, 0);

    nodeA.AddPeerAddress("localhost:" + std::to_string(nodeB.ListenPort()));

    std::atomic<bool> stop{false};
    std::thread tA(RunIo, std::ref(ioA), std::ref(stop));
    std::thread tB(RunIo, std::ref(ioB), std::ref(stop));

    nodeA.Start();
    nodeB.Start();

    auto waitForPeers = [](net::P2PNode& n, size_t expected) {
        for (int i = 0; i < 200; ++i) {
            if (n.Peers().size() >= expected) return true;
            std::this_thread::sleep_for(10ms);
        }
        return false;
    };

    ASSERT_TRUE(waitForPeers(nodeB, 1));
    std::this_thread::sleep_for(800ms);

    EXPECT_EQ(nodeB.Peers().size(), 1u);
    EXPECT_EQ(nodeA.Peers().size(), 1u);

    stop = true;
    tA.join();
    tB.join();

    nodeA.Stop();
    nodeB.Stop();
}
