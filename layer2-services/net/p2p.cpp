#include "p2p.h"

#include <array>
#include <chrono>
#include <cstring>
#include <deque>
#include <optional>
#include <random>

namespace net {

using boost::asio::ip::tcp;

struct P2PNode::PeerState {
    tcp::socket socket;
    PeerInfo info;
    std::deque<Message> outbound;
    int banScore{0};
    size_t msgsThisMinute{0};
    std::chrono::steady_clock::time_point windowStart{std::chrono::steady_clock::now()};
    bool gotVersion{false};
    bool sentVerack{false};

    explicit PeerState(boost::asio::io_context& io, PeerInfo p)
        : socket(io), info(std::move(p)) {}
};

P2PNode::P2PNode(boost::asio::io_context& io, uint16_t listenPort)
    : m_io(io), m_acceptor(io, tcp::endpoint(tcp::v4(), listenPort)), m_timer(io)
{
}

P2PNode::~P2PNode()
{
    Stop();
}

void P2PNode::RegisterHandler(const std::string& cmd, Handler h)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_handlers[cmd] = std::move(h);
}

void P2PNode::AddPeerAddress(const std::string& address)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_seedAddrs.insert(address);
}

void P2PNode::SetLocalHeight(uint32_t height)
{
    m_localHeight = height;
}

void P2PNode::SetTxProvider(PayloadProvider provider)
{
    m_txProvider = std::move(provider);
}

void P2PNode::SetBlockProvider(PayloadProvider provider)
{
    m_blockProvider = std::move(provider);
}

void P2PNode::Start()
{
    LoadDNSSeeds();
    AcceptLoop();
    ConnectSeeds();
    ScheduleHeartbeat();
}

void P2PNode::Stop()
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto& kv : m_peers) {
        boost::system::error_code ec;
        kv.second->socket.close(ec);
    }
    m_peers.clear();
}

void P2PNode::Broadcast(const Message& msg)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto& kv : m_peers) QueueMessage(*kv.second, msg);
}

void P2PNode::SendTo(const std::string& peerId, const Message& msg)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) QueueMessage(*it->second, msg);
}

void P2PNode::AnnounceInventory(const std::vector<uint256>& txs, const std::vector<uint256>& blocks)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto& kv : m_peers) {
        if (!txs.empty()) SendInv(kv.second, txs, /*type=*/0x01);
        if (!blocks.empty()) SendInv(kv.second, blocks, /*type=*/0x02);
    }
}

std::vector<PeerInfo> P2PNode::Peers() const
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::vector<PeerInfo> out;
    for (const auto& kv : m_peers) out.push_back(kv.second->info);
    return out;
}

void P2PNode::AcceptLoop()
{
    auto peer = std::make_shared<PeerState>(m_io, PeerInfo{"", "", true});
    m_acceptor.async_accept(peer->socket, [this, peer](const boost::system::error_code& ec) {
        if (!ec) {
            auto ep = peer->socket.remote_endpoint();
            peer->info.address = ep.address().to_string();
            if (IsBanned(peer->info.address)) { DropPeer(peer->info.id); return; }
            peer->info.id = peer->info.address + ":" + std::to_string(ep.port());
            RegisterPeer(peer);
            SendVersion(peer);
            ReadLoop(peer);
        }
        AcceptLoop();
    });
}

void P2PNode::ConnectSeeds()
{
    for (const auto& addr : m_seedAddrs) {
        auto colon = addr.find(':');
        if (colon == std::string::npos) continue;
        auto host = addr.substr(0, colon);
        auto port = addr.substr(colon + 1);
        auto peer = std::make_shared<PeerState>(m_io, PeerInfo{addr, addr, false});
        tcp::resolver resolver(m_io);
        auto endpoints = resolver.resolve(host, port);
        boost::asio::async_connect(peer->socket, endpoints,
            [this, peer](const boost::system::error_code& ec, const tcp::endpoint&) {
                if (!ec) {
                    RegisterPeer(peer);
                    ReadLoop(peer);
                    SendVersion(peer);
                }
            });
    }
}

void P2PNode::RegisterPeer(const std::shared_ptr<PeerState>& peer)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_peers[peer->info.id] = peer;
}

void P2PNode::QueueMessage(PeerState& peer, const Message& msg)
{
    peer.outbound.push_back(msg);
    if (peer.outbound.size() == 1) WriteLoop(peer);
}

void P2PNode::WriteLoop(PeerState& peer)
{
    if (peer.outbound.empty()) return;
    auto msg = peer.outbound.front();
    std::array<char, 12> cmdBuf{};
    std::copy_n(msg.command.begin(), std::min<size_t>(msg.command.size(), cmdBuf.size()), cmdBuf.begin());
    uint32_t len = static_cast<uint32_t>(msg.payload.size());

    std::vector<uint8_t> frame(sizeof(len) + cmdBuf.size() + msg.payload.size());
    std::memcpy(frame.data(), &len, sizeof(len));
    std::memcpy(frame.data() + sizeof(len), cmdBuf.data(), cmdBuf.size());
    if (!msg.payload.empty()) {
        std::memcpy(frame.data() + sizeof(len) + cmdBuf.size(), msg.payload.data(), msg.payload.size());
    }

    auto self = m_peers[peer.info.id];
    boost::asio::async_write(peer.socket, boost::asio::buffer(frame),
        [this, self](const boost::system::error_code& ec, std::size_t) {
            if (!ec) {
                self->outbound.pop_front();
                WriteLoop(*self);
            } else {
                DropPeer(self->info.id);
            }
        });
}

void P2PNode::ReadLoop(const std::shared_ptr<PeerState>& peer)
{
    auto lenBuf = std::make_shared<std::array<uint8_t, 4>>();
    boost::asio::async_read(peer->socket, boost::asio::buffer(*lenBuf),
        [this, peer, lenBuf](const boost::system::error_code& ec, std::size_t) {
            if (ec) { DropPeer(peer->info.id); return; }
            uint32_t len{0};
            std::memcpy(&len, lenBuf->data(), sizeof(len));
            auto cmdBuf = std::make_shared<std::array<char, 12>>();
            boost::asio::async_read(peer->socket, boost::asio::buffer(*cmdBuf),
                [this, peer, len, cmdBuf](const boost::system::error_code& ec2, std::size_t) {
                    if (ec2) { DropPeer(peer->info.id); return; }
                    auto payload = std::make_shared<std::vector<uint8_t>>(len);
                    boost::asio::async_read(peer->socket, boost::asio::buffer(*payload),
                        [this, peer, cmdBuf, payload](const boost::system::error_code& ec3, std::size_t) {
                            if (ec3) { DropPeer(peer->info.id); return; }
                            Message msg;
                            msg.command.assign(cmdBuf->data(), cmdBuf->data() + cmdBuf->size());
                            while (!msg.command.empty() && msg.command.back() == '\0') msg.command.pop_back();
                            msg.payload = std::move(*payload);
                            if (!RateLimit(*peer)) { DropPeer(peer->info.id); return; }
                            if (msg.command == "ping") {
                                QueueMessage(*peer, Message{"pong", msg.payload});
                            } else if (msg.command == "pong") {
                                // heartbeat reply
                            } else {
                                Dispatch(*peer, msg);
                            }
                            ReadLoop(peer);
                        });
                });
        });
}

void P2PNode::Dispatch(const PeerState& peer, const Message& msg)
{
    if (auto self = m_peers[peer.info.id]) {
        HandleBuiltin(self, msg);
    }
    Handler h;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        auto it = m_handlers.find(msg.command);
        if (it != m_handlers.end()) h = it->second;
    }
    if (h) h(peer.info, msg);
}

bool P2PNode::RateLimit(PeerState& peer)
{
    auto now = std::chrono::steady_clock::now();
    if (now - peer.windowStart > std::chrono::minutes(1)) {
        peer.windowStart = now;
        peer.msgsThisMinute = 0;
    }
    peer.msgsThisMinute++;
    if (peer.msgsThisMinute > m_maxMsgsPerMinute) peer.banScore += 10;
    if (peer.banScore > m_banThreshold) {
        Ban(peer.info.address);
        return false;
    }
    return true;
}

void P2PNode::SendVersion(const std::shared_ptr<PeerState>& peer)
{
    std::string nodeId = peer->info.id.empty() ? "" : peer->info.id;
    const uint32_t version = 1;
    std::vector<uint8_t> payload;
    payload.resize(sizeof(version) + sizeof(m_localHeight) + nodeId.size());
    std::memcpy(payload.data(), &version, sizeof(version));
    std::memcpy(payload.data() + sizeof(version), &m_localHeight, sizeof(m_localHeight));
    std::memcpy(payload.data() + sizeof(version) + sizeof(m_localHeight), nodeId.data(), nodeId.size());
    QueueMessage(*peer, Message{"version", payload});
}

void P2PNode::CompleteHandshake(const std::shared_ptr<PeerState>& peer, uint32_t)
{
    if (!peer->sentVerack) {
        QueueMessage(*peer, Message{"verack", {}});
        peer->sentVerack = true;
    }
    peer->gotVersion = true;
    std::vector<uint8_t> payload(nodeId.begin(), nodeId.end());
    QueueMessage(*peer, Message{"version", payload});
}

void P2PNode::DropPeer(const std::string& id)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_peers.find(id);
    if (it != m_peers.end()) {
        boost::system::error_code ec;
        it->second->socket.close(ec);
        m_peers.erase(it);
    }
}

void P2PNode::Ban(const std::string& address)
{
    m_banned[address] = std::chrono::steady_clock::now() + m_banTime;
}

bool P2PNode::IsBanned(const std::string& address) const
{
    auto it = m_banned.find(address);
    if (it == m_banned.end()) return false;
    if (std::chrono::steady_clock::now() > it->second) return false;
    return true;
}

void P2PNode::ScheduleHeartbeat()

{
    m_timer.expires_after(std::chrono::seconds(30));
    m_timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            Broadcast(Message{"ping", {}});
            ScheduleHeartbeat();
        }
    });
}

void P2PNode::LoadDNSSeeds()
{
    try {
        boost::property_tree::ptree pt;
        read_json("testnet/seeds.json", pt);
        for (const auto& child : pt) {
            auto host = child.second.get<std::string>("host", "");
            auto port = child.second.get<uint16_t>("port", 0);
            if (!host.empty() && port != 0) {
                AddPeerAddress(host + ":" + std::to_string(port));
            }
        for (const auto& child : pt.get_child("seeds")) {
            AddPeerAddress(child.second.get_value<std::string>());
        }
    } catch (...) {
        // ignore missing seeds
    }
}

void P2PNode::HandleBuiltin(const std::shared_ptr<PeerState>& peer, const Message& msg)
{
    if (msg.command == "version") {
        if (msg.payload.size() >= 8) {
            uint32_t version{0};
            uint32_t height{0};
            std::memcpy(&version, msg.payload.data(), sizeof(version));
            std::memcpy(&height, msg.payload.data() + sizeof(version), sizeof(height));
            if (version == 0) { Ban(peer->info.address); DropPeer(peer->info.id); return; }
            CompleteHandshake(peer, height);
        } else {
            Ban(peer->info.address);
            DropPeer(peer->info.id);
            return;
        }
        peer->gotVersion = true;
        QueueMessage(*peer, Message{"verack", {}});
    } else if (msg.command == "verack") {
        peer->sentVerack = true;
    } else if (msg.command == "inv") {
        std::vector<uint256> invs;
        uint8_t type = 0x01;
        size_t stride = 32;
        if (msg.payload.size() % 33 == 0) { stride = 33; }
        for (size_t i = 0; i + stride <= msg.payload.size(); i += stride) {
            uint256 h{};
            if (stride == 33) type = msg.payload[i];
            std::copy(msg.payload.begin() + i + (stride - 32), msg.payload.begin() + i + stride, h.begin());
            if (m_seenInventory.insert(h).second) invs.push_back(h);
        }
        if (!invs.empty()) SendGetData(peer, invs, type);
    } else if (msg.command == "getdata") {
        std::vector<uint256> requests;
        uint8_t type = 0x01;
        size_t stride = 32;
        if (msg.payload.size() % 33 == 0) stride = 33;
        for (size_t i = 0; i + stride <= msg.payload.size(); i += stride) {
            uint256 h{};
            if (stride == 33) type = msg.payload[i];
            std::copy(msg.payload.begin() + i + (stride - 32), msg.payload.begin() + i + stride, h.begin());
            requests.push_back(h);
        }
        for (const auto& h : requests) {
            std::optional<std::vector<uint8_t>> payload;
            if (type == 0x02 && m_blockProvider) payload = m_blockProvider(h);
            if (!payload && m_txProvider) payload = m_txProvider(h);
            if (payload) {
                SendPayload(peer, type == 0x02 ? "block" : "tx", *payload);
            }
        }
    }
}

void P2PNode::SendInv(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& invs, uint8_t type)
{
    std::vector<uint8_t> payload;
    payload.reserve(invs.size() * 33);
    for (const auto& h : invs) {
        payload.push_back(type);
        payload.insert(payload.end(), h.begin(), h.end());
    }
    QueueMessage(*peer, Message{"inv", payload});
}

void P2PNode::SendGetData(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& hashes, uint8_t type)
{
    std::vector<uint8_t> payload;
    payload.reserve(hashes.size() * 33);
    for (const auto& h : hashes) {
        payload.push_back(type);
        payload.insert(payload.end(), h.begin(), h.end());
    }
    QueueMessage(*peer, Message{"getdata", payload});
}

void P2PNode::SendPayload(const std::shared_ptr<PeerState>& peer, const std::string& cmd, const std::vector<uint8_t>& payload)
{
    QueueMessage(*peer, Message{cmd, payload});
}

        for (size_t i = 0; i + 32 <= msg.payload.size(); i += 32) {
            uint256 h{};
            std::copy(msg.payload.begin() + i, msg.payload.begin() + i + 32, h.begin());
            if (m_seenInventory.insert(h).second) invs.push_back(h);
        }
        if (!invs.empty()) SendGetData(peer, invs);
    }
}

void P2PNode::SendInv(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& invs)
{
    std::vector<uint8_t> payload;
    payload.reserve(invs.size() * 32);
    for (const auto& h : invs) payload.insert(payload.end(), h.begin(), h.end());
    QueueMessage(*peer, Message{"inv", payload});
}

void P2PNode::SendGetData(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& hashes)
{
    std::vector<uint8_t> payload;
    payload.reserve(hashes.size() * 32);
    for (const auto& h : hashes) payload.insert(payload.end(), h.begin(), h.end());
    QueueMessage(*peer, Message{"getdata", payload});
}

} // namespace net
