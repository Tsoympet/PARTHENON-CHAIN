#pragma once

#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../layer1-core/crypto/tagged_hash.h"

namespace net {

struct Message {
    std::string command;           // 12 byte command string (ASCII, null padded)
    std::vector<uint8_t> payload;  // raw payload
};

struct PeerInfo {
    std::string id;      // resolved host:port
    std::string address; // ip string
    std::string seed_id; // original seed host:port
    bool inbound{false};
};

struct BloomFilter {
    std::vector<uint8_t> bits;
    uint32_t nHashFuncs{0};
    uint32_t tweak{0};
    bool full{true};

    bool Empty() const { return bits.empty(); }
    bool Match(const uint256& h) const;
};

class P2PNetwork {
public:
    using Handler = std::function<void(const PeerInfo&, const Message&)>;
    using PayloadProvider = std::function<std::optional<std::vector<uint8_t>>(const uint256&)>;

    explicit P2PNetwork(boost::asio::io_context& io, uint16_t listenPort);
    ~P2PNetwork();

    void RegisterHandler(const std::string& cmd, Handler h);
    void AddPeerAddress(const std::string& address);
    void Start();
    void Stop();
    void connect_to_peers();
    void handle_incoming();
    uint16_t ListenPort() const;
    void Broadcast(const Message& msg);
    void SendTo(const std::string& peerId, const Message& msg);
    std::vector<PeerInfo> Peers() const;
    void SetLocalHeight(uint32_t height);
    void SetTxProvider(PayloadProvider provider);
    void SetBlockProvider(PayloadProvider provider);
    void AnnounceInventory(const std::vector<uint256>& txs, const std::vector<uint256>& blocks = {});

private:
    struct PeerState;

    static constexpr uint32_t k_message_magic = 0xd1a0c0deU;

    void AcceptLoop();
    void ConnectSeeds();
    void LoadDNSSeeds();
    void RegisterPeer(const std::shared_ptr<PeerState>& peer);
    void QueueMessage(const std::shared_ptr<PeerState>& peer, const Message& msg);
    void WriteLoop(const std::shared_ptr<PeerState>& peer);
    void ReadLoop(const std::shared_ptr<PeerState>& peer);
    void Dispatch(const PeerState& peer, const Message& msg);
    bool RateLimit(PeerState& peer);
    void SendVersion(const std::shared_ptr<PeerState>& peer);
    void CompleteHandshake(const std::shared_ptr<PeerState>& peer, uint32_t remoteHeight, const std::string& remoteId);
    void DropPeer(const std::string& id);
    void Ban(const std::string& address);
    bool IsBanned(const std::string& address) const;
    void HandleBuiltin(const std::shared_ptr<PeerState>& peer, const Message& msg);
    void SendInv(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& invs, uint8_t type);
    void SendGetData(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& hashes, uint8_t type);
    void SendPayload(const std::shared_ptr<PeerState>& peer, const std::string& cmd, const std::vector<uint8_t>& payload);
    void ScheduleHeartbeat();
    bool ApplyBloom(const PeerState& peer, const uint256& hash) const;

    boost::asio::io_context& m_io;
    boost::asio::ip::tcp::acceptor m_acceptor;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<PeerState>> m_peers;
    std::unordered_map<std::string, Handler> m_handlers;
    std::set<std::string> m_seedAddrs;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_banned;
    std::set<uint256> m_seenInventory;
    boost::asio::steady_timer m_timer;
    boost::asio::steady_timer m_seedTimer;
    PayloadProvider m_txProvider;
    PayloadProvider m_blockProvider;
    uint32_t m_localHeight{0};
    const size_t m_maxMsgsPerMinute{200};
    const size_t m_maxPeers{64};
    const int m_banThreshold{100};
    const std::chrono::minutes m_banTime{10};
    bool m_stopped{false};
};

using P2PNode = P2PNetwork; // backward compatibility for existing call sites

} // namespace net
