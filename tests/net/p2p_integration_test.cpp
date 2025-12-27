#include "../../layer2-services/net/p2p.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>

int main()
{
    boost::asio::io_context ioA;
    boost::asio::io_context ioB;

    net::P2PNode nodeA(ioA, 19330);
    net::P2PNode nodeB(ioB, 19331);

    std::atomic<int> received{0};
    nodeB.RegisterHandler("hello", [&received](const net::PeerInfo&, const net::Message& msg) {
        if (msg.payload.size() == 3 && msg.payload[0] == 0xAA) {
            received.fetch_add(1, std::memory_order_relaxed);
        }
    });

    nodeA.AddPeerAddress("127.0.0.1:19331");
    nodeB.AddPeerAddress("127.0.0.1:19330");

    nodeA.Start();
    nodeB.Start();

    // Allow connect handshake to occur.
    for (int i = 0; i < 5; ++i) {
        ioA.run_for(std::chrono::milliseconds(100));
        ioB.run_for(std::chrono::milliseconds(100));
        ioA.restart();
        ioB.restart();
    }

    net::Message hello{"hello", {0xAA, 0xBB, 0xCC}};
    nodeA.Broadcast(hello);

    for (int i = 0; i < 5; ++i) {
        ioA.run_for(std::chrono::milliseconds(100));
        ioB.run_for(std::chrono::milliseconds(100));
        ioA.restart();
        ioB.restart();
    }

    auto peersA = nodeA.Peers();
    auto peersB = nodeB.Peers();
    assert(!peersA.empty());
    assert(!peersB.empty());
    assert(received.load(std::memory_order_relaxed) > 0);

    nodeA.Stop();
    nodeB.Stop();
    return 0;
}
