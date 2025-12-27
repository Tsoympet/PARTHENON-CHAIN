#include "p2p.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <deque>
#include <optional>
#include <random>

namespace net {

using boost::asio::ip::tcp;

static std::string PadCommand(const std::string& cmd)
{
    std::array<char, 12> buf{};
    std::memset(buf.data(), 0, buf.size());
    std::memcpy(buf.data(), cmd.data(), std::min(cmd.size(), buf.size()));
    return std::string(buf.data(), buf.size());
}

bool BloomFilter::Match(const uint256& h) const
{
    if (full || bits.empty()) return true;
    const size_t bitCount = bits.size() * 8;
    auto hashToInt = [&h](uint32_t seed) {
        uint32_t out = seed;
        for (auto b : h) {
            out = out * 0x01000193 ^ b;
        }
        return out;
    };
    for (uint32_t i = 0; i < nHashFuncs; ++i) {
        uint32_t hv = hashToInt(i * 0xfba4c795 + tweak);
        size_t bit = hv % bitCount;
        size_t byteIdx = bit / 8;
        uint8_t mask = 1u << (bit % 8);
        if ((bits[byteIdx] & mask) == 0) return false;
    }
    return true;
}

struct P2PNode::PeerState {
    tcp::socket socket;
    PeerInfo info;
    std::deque<Message> outbound;
    int banScore{0};
    size_t msgsThisMinute{0};
    std::chrono::steady_clock::time_point windowStart{std::chrono::steady_clock::now()};
    bool gotVersion{false};
    bool sentVerack{false};
    BloomFilter filter{};

    explicit PeerState(boost::asio::io_context& io, PeerInfo p)
        : socket(io), info(std::move(p)) {}
};

P2PNode::P2PNode(boost::asio::io_context& io, uint16_t listenPort)
    : m_io(io), m_acceptor(io), m_timer(io)
{
    tcp::endpoint ep(tcp::v6(), listenPort);
    boost::system::error_code ec;
    m_acceptor.open(ep.protocol(), ec);
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
    m_acceptor.set_option(boost::asio::ip::v6_only(false));
    m_acceptor.bind(ep, ec);
    if (ec) {
        // fallback to v4 only
        m_acceptor.open(tcp::v4());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind({tcp::v4(), listenPort});
    }
    m_acceptor.listen();
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

uint16_t P2PNode::ListenPort() const
{
    boost::system::error_code ec;
    auto ep = m_acceptor.local_endpoint(ec);
    return ec ? 0 : ep.port();
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
            peer->info.id = peer->info.address + ":" + std::to_string(ep.port());
            if (IsBanned(peer->info.address)) { DropPeer(peer->info.id); return; }
            RegisterPeer(peer);
            SendVersion(peer);
            ReadLoop(peer);
        }
        AcceptLoop();
    });
}

void P2PNode::ConnectSeeds()
{
    std::vector<std::string> seeds;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        seeds.assign(m_seedAddrs.begin(), m_seedAddrs.end());
    }
    for (const auto& addr : seeds) {
        auto colon = addr.find(':');
        if (colon == std::string::npos) continue;
        auto host = addr.substr(0, colon);
        auto port = addr.substr(colon + 1);
        auto peer = std::make_shared<PeerState>(m_io, PeerInfo{"", host, false});
        tcp::resolver resolver(m_io);
        resolver.async_resolve(host, port, [this, peer](const boost::system::error_code& ec, tcp::resolver::results_type res) {
            if (ec) return;
            boost::asio::async_connect(peer->socket, res, [this, peer](const boost::system::error_code& ec2, const tcp::endpoint& ep) {
                if (ec2) return;
                peer->info.address = ep.address().to_string();
                peer->info.id = peer->info.address + ":" + std::to_string(ep.port());
                if (IsBanned(peer->info.address)) { DropPeer(peer->info.id); return; }
                RegisterPeer(peer);
                SendVersion(peer);
                ReadLoop(peer);
            });
        });
    }
}

void P2PNode::LoadDNSSeeds()
{
    // mainnet preferred, fallback to testnet
    for (const auto& path : {"mainnet/seeds.json", "testnet/seeds.json"}) {
        try {
            boost::property_tree::ptree pt;
            read_json(path, pt);
            if (auto seeds = pt.get_child_optional("seeds")) {
                for (const auto& child : *seeds) {
                    AddPeerAddress(child.second.get_value<std::string>());
                }
            }
            for (const auto& child : pt) {
                if (child.first == "seeds") continue;
                auto host = child.second.get<std::string>("host", "");
                auto port = child.second.get<uint16_t>("port", 0);
                if (!host.empty() && port != 0) {
                    AddPeerAddress(host + ":" + std::to_string(port));
                }
            }
        } catch (...) {
            // ignore missing file
        }
    }
}

void P2PNode::RegisterPeer(const std::shared_ptr<PeerState>& peer)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_peers[peer->info.id] = peer;
    WriteLoop(*peer);
}

void P2PNode::QueueMessage(PeerState& peer, const Message& msg)
{
    peer.outbound.push_back(msg);
    if (peer.outbound.size() == 1) {
        WriteLoop(peer);
    }
}

void P2PNode::WriteLoop(PeerState& peer)
{
    if (peer.outbound.empty()) return;
    const Message msg = peer.outbound.front();
    std::string cmdPadded = PadCommand(msg.command);
    uint32_t len = static_cast<uint32_t>(msg.payload.size());
    std::array<uint8_t, 16> header{};
    std::memcpy(header.data(), cmdPadded.data(), 12);
    std::memcpy(header.data() + 12, &len, sizeof(len));
    std::array<boost::asio::const_buffer, 2> bufs{
        boost::asio::buffer(header),
        boost::asio::buffer(msg.payload)
    };
    boost::asio::async_write(peer.socket, bufs, [this, &peer](const boost::system::error_code& ec, std::size_t) {
        if (ec) { DropPeer(peer.info.id); return; }
        if (!peer.outbound.empty()) peer.outbound.pop_front();
        WriteLoop(peer);
    });
}

void P2PNode::ReadLoop(const std::shared_ptr<PeerState>& peer)
{
    auto header = std::make_shared<std::array<uint8_t, 16>>();
    boost::asio::async_read(peer->socket, boost::asio::buffer(*header), [this, peer, header](const boost::system::error_code& ec, std::size_t) {
        if (ec) { DropPeer(peer->info.id); return; }
        std::string cmd(reinterpret_cast<char*>(header->data()), 12);
        while (!cmd.empty() && cmd.back() == '\0') cmd.pop_back();
        uint32_t len{0};
        std::memcpy(&len, header->data() + 12, sizeof(len));
        auto payload = std::make_shared<std::vector<uint8_t>>(len);
        boost::asio::async_read(peer->socket, boost::asio::buffer(*payload), [this, peer, cmd, payload](const boost::system::error_code& ec2, std::size_t) {
            if (ec2) { DropPeer(peer->info.id); return; }
            Message msg{cmd, std::move(*payload)};
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
    const uint32_t version = 1;
    std::string nodeId = peer->info.id.empty() ? "" : peer->info.id;
    std::vector<uint8_t> payload;
    payload.resize(sizeof(version) + sizeof(m_localHeight) + nodeId.size());
    std::memcpy(payload.data(), &version, sizeof(version));
    std::memcpy(payload.data() + sizeof(version), &m_localHeight, sizeof(m_localHeight));
    std::memcpy(payload.data() + sizeof(version) + sizeof(m_localHeight), nodeId.data(), nodeId.size());
    QueueMessage(*peer, Message{"version", payload});
}

void P2PNode::CompleteHandshake(const std::shared_ptr<PeerState>& peer, uint32_t remoteHeight, const std::string& remoteId)
{
    (void)remoteHeight; // reserved for future sync logic
    peer->info.id = remoteId.empty() ? peer->info.id : remoteId;
    peer->gotVersion = true;
    if (!peer->sentVerack) {
        QueueMessage(*peer, Message{"verack", {}});
        peer->sentVerack = true;
    }
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

bool P2PNode::ApplyBloom(const PeerState& peer, const uint256& hash) const
{
    if (peer.filter.Empty()) return true;
    return peer.filter.Match(hash);
}

void P2PNode::HandleBuiltin(const std::shared_ptr<PeerState>& peer, const Message& msg)
{
    if (msg.command == "version") {
        if (msg.payload.size() >= 8) {
            uint32_t version{0};
            uint32_t height{0};
            std::memcpy(&version, msg.payload.data(), sizeof(version));
            std::memcpy(&height, msg.payload.data() + sizeof(version), sizeof(height));
            std::string remoteId;
            if (msg.payload.size() > 8) {
                remoteId.assign(reinterpret_cast<const char*>(msg.payload.data() + 8), msg.payload.size() - 8);
            }
            if (version == 0) { Ban(peer->info.address); DropPeer(peer->info.id); return; }
            CompleteHandshake(peer, height, remoteId);
        } else {
            Ban(peer->info.address);
            DropPeer(peer->info.id);
            return;
        }
        QueueMessage(*peer, Message{"verack", {}});
    } else if (msg.command == "verack") {
        peer->sentVerack = true;
    } else if (msg.command == "filterload") {
        // payload: [nHashFuncs(4)][tweak(4)][data...]
        if (msg.payload.size() >= 8) {
            BloomFilter bf;
            std::memcpy(&bf.nHashFuncs, msg.payload.data(), sizeof(bf.nHashFuncs));
            std::memcpy(&bf.tweak, msg.payload.data() + 4, sizeof(bf.tweak));
            bf.bits.assign(msg.payload.begin() + 8, msg.payload.end());
            bf.full = false;
            peer->filter = std::move(bf);
        }
    } else if (msg.command == "filteradd") {
        if (peer->filter.Empty()) return;
        if (msg.payload.size() >= 32) {
            uint256 h{};
            std::copy(msg.payload.begin(), msg.payload.begin() + 32, h.begin());
            // set bits
            const size_t bitCount = peer->filter.bits.size() * 8;
            auto hashToInt = [&h](uint32_t seed) {
                uint32_t out = seed;
                for (auto b : h) out = out * 0x01000193 ^ b;
                return out;
            };
            for (uint32_t i = 0; i < peer->filter.nHashFuncs; ++i) {
                uint32_t hv = hashToInt(i * 0xfba4c795 + peer->filter.tweak);
                size_t bit = hv % bitCount;
                peer->filter.bits[bit / 8] |= (1u << (bit % 8));
            }
        }
    } else if (msg.command == "filterclear") {
        peer->filter = BloomFilter{};
    } else if (msg.command == "inv") {
        std::vector<uint256> invs;
        uint8_t type = 0x01;
        size_t stride = (msg.payload.size() % 33 == 0) ? 33 : 32;
        for (size_t i = 0; i + stride <= msg.payload.size(); i += stride) {
            uint256 h{};
            if (stride == 33) type = msg.payload[i];
            std::copy(msg.payload.begin() + i + (stride - 32), msg.payload.begin() + i + stride, h.begin());
            if (ApplyBloom(*peer, h) && m_seenInventory.insert(h).second) invs.push_back(h);
        }
        if (!invs.empty()) SendGetData(peer, invs, type);
    } else if (msg.command == "getdata") {
        std::vector<uint256> requests;
        uint8_t type = 0x01;
        size_t stride = (msg.payload.size() % 33 == 0) ? 33 : 32;
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

