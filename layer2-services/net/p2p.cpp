#include "p2p.h"

#include <array>
#include <cstring>
#include <deque>
#include <optional>

namespace net {

using boost::asio::ip::tcp;

struct P2PNode::PeerState {
    tcp::socket socket;
    PeerInfo info;
    std::deque<Message> outbound;
    int banScore{0};
    size_t msgsThisMinute{0};
    std::chrono::steady_clock::time_point windowStart{std::chrono::steady_clock::now()};

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

void P2PNode::Start()
{
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
            peer->info.address = peer->socket.remote_endpoint().address().to_string();
            peer->info.id = peer->info.address + ":" + std::to_string(peer->socket.remote_endpoint().port());
            RegisterPeer(peer);
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
                            Dispatch(*peer, msg);
                            ReadLoop(peer);
                        });
                });
        });
}

void P2PNode::Dispatch(const PeerState& peer, const Message& msg)
{
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
    return peer.banScore <= m_banThreshold;
}

void P2PNode::SendVersion(const std::shared_ptr<PeerState>& peer)
{
    Message m{"version", {}};
    QueueMessage(*peer, m);
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

} // namespace net
