#include "p2p.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <deque>
#include <mutex>
#include <optional>
#include <random>
#include <unordered_set>

#include "../../layer1-core/pow/sha256d.h"

namespace net {

using boost::asio::ip::tcp;

// Safely pads command to 12 bytes with null termination
static void PadCommand(const std::string& cmd, std::array<char, 12>& out)
{
    std::memset(out.data(), 0, out.size());
    std::memcpy(out.data(), cmd.data(), std::min(cmd.size(), out.size()));
}

static constexpr size_t k_max_payload = 4 * 1024 * 1024; // 4 MiB safety cap

bool BloomFilter::Match(const uint256& h) const
{
    if (full || bits.empty()) return true;
    const size_t bitCount = bits.size() * 8;
    
    // Compute base hash once instead of in lambda for each iteration
    uint32_t baseHash = tweak;
    for (auto b : h) {
        baseHash = baseHash * 0x01000193 ^ b;
    }
    
    for (uint32_t i = 0; i < nHashFuncs; ++i) {
        // Combine base hash with iteration-specific seed
        uint32_t hv = baseHash ^ (i * 0xfba4c795);
        size_t bit = hv % bitCount;
        size_t byteIdx = bit / 8;
        uint8_t mask = 1u << (bit % 8);
        if ((bits[byteIdx] & mask) == 0) return false;
    }
    return true;
}

struct P2PNetwork::PeerState {
    tcp::socket socket;
    PeerInfo info;
    std::deque<Message> outbound;
    std::mutex outboundMutex;
    int banScore{0};
    size_t msgsThisMinute{0};
    std::chrono::steady_clock::time_point windowStart{std::chrono::steady_clock::now()};
    bool gotVersion{false};
    bool sentVerack{false};
    BloomFilter filter{};

    explicit PeerState(boost::asio::io_context& io, PeerInfo p)
        : socket(io), info(std::move(p)) {}
};

P2PNetwork::P2PNetwork(boost::asio::io_context& io, uint16_t listenPort)
    : m_io(io), m_acceptor(io), m_timer(io), m_seedTimer(io)
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

P2PNetwork::~P2PNetwork()
{
    Stop();
}

void P2PNetwork::RegisterHandler(const std::string& cmd, Handler h)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_handlers[cmd] = std::move(h);
}

void P2PNetwork::AddPeerAddress(const std::string& address)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_seedAddrs.insert(address);
}

void P2PNetwork::SetLocalHeight(uint32_t height)
{
    m_localHeight = height;
}

void P2PNetwork::SetTxProvider(PayloadProvider provider)
{
    m_txProvider = std::move(provider);
}

void P2PNetwork::SetBlockProvider(PayloadProvider provider)
{
    m_blockProvider = std::move(provider);
}

void P2PNetwork::Start()
{
    LoadDNSSeeds();
    AcceptLoop();
    ConnectSeeds();
    ScheduleHeartbeat();
}

void P2PNetwork::connect_to_peers()
{
    ConnectSeeds();
}

void P2PNetwork::handle_incoming()
{
    AcceptLoop();
}

void P2PNetwork::Stop()
{
    m_stopped = true;
    std::lock_guard<std::mutex> g(m_mutex);
    boost::system::error_code ec;
    m_timer.cancel(ec);
    m_seedTimer.cancel(ec);
    m_acceptor.close(ec);
    for (auto& kv : m_peers) {
        kv.second->socket.close(ec);
    }
    m_peers.clear();
    m_seenInventory.clear();
    m_io.poll();
}

uint16_t P2PNetwork::ListenPort() const
{
    boost::system::error_code ec;
    auto ep = m_acceptor.local_endpoint(ec);
    return ec ? 0 : ep.port();
}

void P2PNetwork::Broadcast(const Message& msg)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto& kv : m_peers) QueueMessage(kv.second, msg);
}

void P2PNetwork::SendTo(const std::string& peerId, const Message& msg)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_peers.find(peerId);
    if (it != m_peers.end()) QueueMessage(it->second, msg);
}

void P2PNetwork::AnnounceInventory(const std::vector<uint256>& txs, const std::vector<uint256>& blocks)
{
    std::lock_guard<std::mutex> g(m_mutex);
    for (auto& kv : m_peers) {
        if (!txs.empty()) SendInv(kv.second, txs, /*type=*/0x01);
        if (!blocks.empty()) SendInv(kv.second, blocks, /*type=*/0x02);
        if (m_txProvider) {
            for (const auto& h : txs) {
                if (auto payload = m_txProvider(h)) {
                    QueueMessage(kv.second, Message{"tx", *payload});
                }
            }
        }
    }
}

std::vector<PeerInfo> P2PNetwork::Peers() const
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::vector<PeerInfo> out;
    for (const auto& kv : m_peers) out.push_back(kv.second->info);
    return out;
}

void P2PNetwork::AcceptLoop()
{
    if (m_stopped) return;
    auto peer = std::make_shared<PeerState>(m_io, PeerInfo{"", "", "", true});
    m_acceptor.async_accept(peer->socket, [this, peer](const boost::system::error_code& ec) {
        if (m_stopped) return;
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

void P2PNetwork::ConnectSeeds()
{
    if (m_stopped) return;
    std::vector<std::string> seeds;
    std::unordered_set<std::string> connectedAddrs;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        seeds.assign(m_seedAddrs.begin(), m_seedAddrs.end());
        // Build hash set of connected addresses for O(1) lookup
        for (const auto& kv : m_peers) {
            if (kv.second) {
                const auto& info = kv.second->info;
                connectedAddrs.insert(info.seed_id.empty() ? info.id : info.seed_id);
            }
        }
    }
    for (const auto& addr : seeds) {
        // Skip if already connected
        if (connectedAddrs.count(addr)) continue;
        
        auto colon = addr.find(':');
        if (colon == std::string::npos) continue;
        auto host = addr.substr(0, colon);
        auto port = addr.substr(colon + 1);
        
        auto peer = std::make_shared<PeerState>(m_io, PeerInfo{"", host, addr, false});
        auto resolver = std::make_shared<tcp::resolver>(m_io);
        resolver->async_resolve(host, port, [this, peer, resolver](const boost::system::error_code& ec, tcp::resolver::results_type res) {
            if (m_stopped || ec) return;
            boost::asio::async_connect(peer->socket, res, [this, peer, resolver](const boost::system::error_code& ec2, const tcp::endpoint& ep) {
                if (m_stopped || ec2) return;
                peer->info.address = ep.address().to_string();
                peer->info.id = peer->info.address + ":" + std::to_string(ep.port());
                if (IsBanned(peer->info.address)) { DropPeer(peer->info.id); return; }
                RegisterPeer(peer);
                SendVersion(peer);
                ReadLoop(peer);
            });
        });
    }
    m_seedTimer.expires_after(std::chrono::milliseconds(200));
    m_seedTimer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && !m_stopped) ConnectSeeds();
    });
}

void P2PNetwork::LoadDNSSeeds()
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

void P2PNetwork::RegisterPeer(const std::shared_ptr<PeerState>& peer)
{
    std::lock_guard<std::mutex> g(m_mutex);
    if (m_peers.size() >= m_maxPeers) {
        boost::system::error_code ec;
        peer->socket.close(ec);
        return;
    }
    m_peers[peer->info.id] = peer;
    WriteLoop(peer);
}

void P2PNetwork::QueueMessage(const std::shared_ptr<PeerState>& peer, const Message& msg)
{
    Message queued = msg;
    boost::asio::post(m_io, [this, peer, queued = std::move(queued)]() mutable {
        std::unique_lock<std::mutex> ql(peer->outboundMutex);
        bool idle = peer->outbound.empty();
        peer->outbound.push_back(std::move(queued));
        ql.unlock();
        if (idle) WriteLoop(peer);
    });
}

void P2PNetwork::WriteLoop(const std::shared_ptr<PeerState>& peer)
{
    Message msg;
    {
        std::lock_guard<std::mutex> ql(peer->outboundMutex);
        if (peer->outbound.empty()) return;
        msg = peer->outbound.front();
    }
    uint32_t len = static_cast<uint32_t>(msg.payload.size());
    std::array<uint8_t, 24> header{};
    std::memcpy(header.data(), &k_message_magic, sizeof(uint32_t));
    std::array<char, 12> cmdBuf;
    PadCommand(msg.command, cmdBuf);
    std::memcpy(header.data() + 4, cmdBuf.data(), cmdBuf.size());
    std::memcpy(header.data() + 16, &len, sizeof(len));
    uint8_t checksumFull[32]{};
    sha256d(checksumFull, msg.payload.empty() ? nullptr : msg.payload.data(), msg.payload.size());
    std::memcpy(header.data() + 20, checksumFull, sizeof(uint32_t));
    std::array<boost::asio::const_buffer, 2> bufs{
        boost::asio::buffer(header),
        boost::asio::buffer(msg.payload)
    };
    boost::asio::async_write(peer->socket, bufs, [this, peer](const boost::system::error_code& ec, std::size_t) {
        if (m_stopped) return;
        if (ec) { DropPeer(peer->info.id); return; }
        bool hasMore{false};
        {
            std::lock_guard<std::mutex> ql(peer->outboundMutex);
            if (!peer->outbound.empty()) peer->outbound.pop_front();
            hasMore = !peer->outbound.empty();
        }
        if (hasMore) WriteLoop(peer);
    });
}

void P2PNetwork::ReadLoop(const std::shared_ptr<PeerState>& peer)
{
    auto header = std::make_shared<std::array<uint8_t, 24>>();
    boost::asio::async_read(peer->socket, boost::asio::buffer(*header), [this, peer, header](const boost::system::error_code& ec, std::size_t) {
        if (m_stopped) return;
        if (ec) { DropPeer(peer->info.id); return; }
        uint32_t magic{0};
        std::memcpy(&magic, header->data(), sizeof(magic));
        if (magic != k_message_magic) { Ban(peer->info.address); DropPeer(peer->info.id); return; }
        std::string cmd(reinterpret_cast<char*>(header->data() + 4), 12);
        while (!cmd.empty() && cmd.back() == '\0') cmd.pop_back();
        uint32_t len{0};
        std::memcpy(&len, header->data() + 16, sizeof(len));
        uint32_t checksum{0};
        std::memcpy(&checksum, header->data() + 20, sizeof(checksum));
        if (len > k_max_payload) { Ban(peer->info.address); DropPeer(peer->info.id); return; }
        auto payload = std::make_shared<std::vector<uint8_t>>(len);
        boost::asio::async_read(peer->socket, boost::asio::buffer(*payload), [this, peer, cmd, payload, checksum](const boost::system::error_code& ec2, std::size_t) {
            if (m_stopped) return;
            if (ec2) { DropPeer(peer->info.id); return; }
            uint8_t verify[32]{};
            sha256d(verify, payload->empty() ? nullptr : payload->data(), payload->size());
            uint32_t calc{0};
            std::memcpy(&calc, verify, sizeof(calc));
            if (calc != checksum) { Ban(peer->info.address); DropPeer(peer->info.id); return; }
            Message msg{cmd, std::move(*payload)};
            if (!RateLimit(*peer)) { DropPeer(peer->info.id); return; }
            if (msg.command == "ping") {
                QueueMessage(peer, Message{"pong", msg.payload});
            } else if (msg.command == "pong") {
                // heartbeat reply
            } else {
                Dispatch(*peer, msg);
            }
            ReadLoop(peer);
        });
    });
}

void P2PNetwork::Dispatch(const PeerState& peer, const Message& msg)
{
    std::shared_ptr<PeerState> self;
    {
        std::lock_guard<std::mutex> g(m_mutex);
        auto it = m_peers.find(peer.info.id);
        if (it != m_peers.end()) self = it->second;
    }
    if (self) {
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

bool P2PNetwork::RateLimit(PeerState& peer)
{
    auto now = std::chrono::steady_clock::now();
    if (now - peer.windowStart > std::chrono::minutes(1)) {
        peer.windowStart = now;
        peer.msgsThisMinute = 0;
    }
    peer.msgsThisMinute++;
    if (peer.msgsThisMinute > m_maxMsgsPerMinute) peer.banScore = m_banThreshold + 1;
    if (peer.banScore > m_banThreshold) {
        Ban(peer.info.address);
        return false;
    }
    return true;
}

void P2PNetwork::SendVersion(const std::shared_ptr<PeerState>& peer)
{
    const uint32_t version = 1;
    std::string nodeId = peer->info.id.empty() ? "" : peer->info.id;
    std::vector<uint8_t> payload;
    payload.resize(sizeof(version) + sizeof(m_localHeight) + nodeId.size());
    std::memcpy(payload.data(), &version, sizeof(version));
    std::memcpy(payload.data() + sizeof(version), &m_localHeight, sizeof(m_localHeight));
    std::memcpy(payload.data() + sizeof(version) + sizeof(m_localHeight), nodeId.data(), nodeId.size());
    QueueMessage(peer, Message{"version", payload});
}

void P2PNetwork::CompleteHandshake(const std::shared_ptr<PeerState>& peer, uint32_t remoteHeight, const std::string& remoteId)
{
    (void)remoteHeight; // reserved for future sync logic
    peer->gotVersion = true;
    if (!peer->sentVerack) {
        QueueMessage(peer, Message{"verack", {}});
        peer->sentVerack = true;
    }
}

void P2PNetwork::DropPeer(const std::string& id)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto it = m_peers.find(id);
    if (it != m_peers.end()) {
        boost::system::error_code ec;
        it->second->socket.close(ec);
        m_peers.erase(it);
    }
}

void P2PNetwork::Ban(const std::string& address)
{
    m_banned[address] = std::chrono::steady_clock::now() + m_banTime;
    std::vector<std::string> toDrop;
    for (const auto& kv : m_peers) {
        if (kv.second && kv.second->info.address == address) {
            toDrop.push_back(kv.first);
        }
    }
    for (const auto& id : toDrop) DropPeer(id);
}

bool P2PNetwork::IsBanned(const std::string& address) const
{
    auto it = m_banned.find(address);
    if (it == m_banned.end()) return false;
    if (std::chrono::steady_clock::now() > it->second) return false;
    return true;
}

bool P2PNetwork::ApplyBloom(const PeerState& peer, const uint256& hash) const
{
    if (peer.filter.Empty()) return true;
    return peer.filter.Match(hash);
}

void P2PNetwork::HandleBuiltin(const std::shared_ptr<PeerState>& peer, const Message& msg)
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
        QueueMessage(peer, Message{"verack", {}});
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
            // set bits - optimized to compute base hash once
            const size_t bitCount = peer->filter.bits.size() * 8;
            uint32_t baseHash = peer->filter.tweak;
            for (auto b : h) {
                baseHash = baseHash * 0x01000193 ^ b;
            }
            for (uint32_t i = 0; i < peer->filter.nHashFuncs; ++i) {
                uint32_t hv = baseHash ^ (i * 0xfba4c795);
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
            {
                std::lock_guard<std::mutex> g(m_mutex);
                if (ApplyBloom(*peer, h) && m_seenInventory.insert(h).second) invs.push_back(h);
            }
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
    } else if (msg.command == "tx") {
        uint256 seenHash{};
        if (msg.payload.size() >= seenHash.size()) {
            std::copy_n(msg.payload.begin(), seenHash.size(), seenHash.begin());
        }
        {
            std::lock_guard<std::mutex> g(m_mutex);
            m_seenInventory.insert(seenHash);
            for (const auto& kv : m_peers) {
                if (kv.first == peer->info.id) continue;
                QueueMessage(kv.second, msg);
            }
        }
    }
}

void P2PNetwork::SendInv(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& invs, uint8_t type)
{
    std::vector<uint8_t> payload;
    payload.reserve(invs.size() * 33);
    for (const auto& h : invs) {
        payload.push_back(type);
        payload.insert(payload.end(), h.begin(), h.end());
    }
    QueueMessage(peer, Message{"inv", payload});
}

void P2PNetwork::SendGetData(const std::shared_ptr<PeerState>& peer, const std::vector<uint256>& hashes, uint8_t type)
{
    std::vector<uint8_t> payload;
    payload.reserve(hashes.size() * 33);
    for (const auto& h : hashes) {
        payload.push_back(type);
        payload.insert(payload.end(), h.begin(), h.end());
    }
    QueueMessage(peer, Message{"getdata", payload});
}

void P2PNetwork::SendPayload(const std::shared_ptr<PeerState>& peer, const std::string& cmd, const std::vector<uint8_t>& payload)
{
    QueueMessage(peer, Message{cmd, payload});
}

void P2PNetwork::ScheduleHeartbeat()
{
    m_timer.expires_after(std::chrono::seconds(30));
    m_timer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec && !m_stopped) {
            Broadcast(Message{"ping", {}});
            ScheduleHeartbeat();
        }
    });
}

} // namespace net
