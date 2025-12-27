#pragma once

#include <boost/asio.hpp>
#include "../../layer1-core/crypto/tagged_hash.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace net {

struct Message {
    std::string command;
    std::vector<uint8_t> payload;
};

struct PeerInfo {
    std::string id;
    std::string address;
    bool inbound{false};
};

class P2PNode {
public:
    using Handler = std::function<void(const PeerInfo&, const Message&)>;

    explicit P2PNode(boost::asio::io_context& io, uint16_t listenPort);
    ~P2PNode();

    void RegisterHandler(const std::string& cmd, Handler h);
    void AddPeerAddress(const std::string& address);
    void Start();
    void Stop();
    void Broadcast(const Message& msg);
    void SendTo(const std::string& peerId, const Message& msg);
    std::vector<PeerInfo> Peers() const;

private:
    struct PeerState;
    void AcceptLoop();
    void ConnectSeeds();
    void LoadDNSSeeds();
    void RegisterPeer(const std::shared_ptr<PeerState>& peer);
    void QueueMessage(PeerState& peer, const Message& msg);
    void WriteLoop(PeerState& peer);
    void ReadLoop(const std::shared_ptr<PeerState>& peer);
    void Dispatch(const PeerState& peer, const Message& msg);
    bool RateLimit(PeerState& peer);
    void SendVersion(const std::shared_ptr<PeerState>& peer);
    void DropPeer(const std::string& id);
    void Ban(const std::string& address);
    bool IsBanned(const std::string& address) const;
    void HandleBuiltin(const std::shared_ptr<PeerState>& peer, const Message& msg);
    void SendInv(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& invs);
    void SendGetData(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& hashes);
    void ScheduleHeartbeat();

    boost::asio::io_context& m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<PeerState>> m_peers;
    std::unordered_map<std::string, Handler> m_handlers;
    std::set<std::string> m_seedAddrs;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_banned;
    std::set<uint256> m_seenInventory;
    boost::asio::steady_timer m_timer;
    const size_t m_maxMsgsPerMinute{600};
    const int m_banThreshold{100};
    const std::chrono::minutes m_banTime{10};
};

} // namespace net
