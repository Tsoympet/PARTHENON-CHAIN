#include <gtest/gtest.h>
#include "../../layer2-services/net/p2p.h"
#include <thread>

TEST(P2PDoS, ExcessTrafficTriggersBan)
{
    boost::asio::io_context ioA;
    boost::asio::io_context ioB;
    net::P2PNode nodeA(ioA, 19550);
    net::P2PNode nodeB(ioB, 19551);

    nodeA.AddPeerAddress("127.0.0.1:19551");
    nodeB.AddPeerAddress("127.0.0.1:19550");

    nodeA.Start();
    nodeB.Start();

    for (int i = 0; i < 5; ++i) {
        ioA.run_for(std::chrono::milliseconds(50));
        ioB.run_for(std::chrono::milliseconds(50));
        ioA.restart();
        ioB.restart();
    }

    // Flood nodeB with messages from nodeA until it bans the peer
    for (int i = 0; i < 650; ++i) {
        nodeA.Broadcast(net::Message{"ping", {0x01}});
    }

    for (int i = 0; i < 10; ++i) {
        ioA.run_for(std::chrono::milliseconds(50));
        ioB.run_for(std::chrono::milliseconds(50));
        ioA.restart();
        ioB.restart();
    }

    auto peersA = nodeA.Peers();
    auto peersB = nodeB.Peers();
    // At least one side should have dropped the connection due to DoS score
    EXPECT_TRUE(peersA.empty() || peersB.empty());

    nodeA.Stop();
    nodeB.Stop();
}
