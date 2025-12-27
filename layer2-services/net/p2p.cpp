#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace net {

struct Message {
    std::string command;
    std::vector<uint8_t> payload;
};

struct PeerInfo {
    std::string id;
    std::string address;
};

class P2PNode {
public:
    using Handler = std::function<void(const PeerInfo&, const Message&)>;

    P2PNode() : m_running(false) {}
    ~P2PNode() { Stop(); }

    void RegisterHandler(const std::string& cmd, Handler h)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_handlers[cmd] = std::move(h);
    }

    void AddPeer(const PeerInfo& peer)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        m_peers[peer.id] = peer;
    }

    bool HasPeer(const std::string& id) const
    {
        std::lock_guard<std::mutex> g(m_mutex);
        return m_peers.count(id) != 0;
    }

    void Broadcast(const Message& msg)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        for (const auto& kv : m_peers)
            m_queue.push({kv.second, msg});
        m_cv.notify_all();
    }

    void SendTo(const std::string& peerId, const Message& msg)
    {
        std::lock_guard<std::mutex> g(m_mutex);
        auto it = m_peers.find(peerId);
        if (it != m_peers.end()) {
            m_queue.push({it->second, msg});
            m_cv.notify_all();
        }
    }

    void Start()
    {
        if (m_running.exchange(true)) return;
        m_worker = std::thread([this]{ this->Process(); });
    }

    void Stop()
    {
        if (!m_running.exchange(false)) return;
        {
            std::lock_guard<std::mutex> g(m_mutex);
            while (!m_queue.empty()) m_queue.pop();
            m_cv.notify_all();
        }
        if (m_worker.joinable()) m_worker.join();
    }

private:
    void Process()
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        while (m_running) {
            if (m_queue.empty()) {
                m_cv.wait(lk);
                continue;
            }
            auto item = m_queue.front();
            m_queue.pop();
            auto handlers = m_handlers; // copy to avoid holding lock during callbacks
            lk.unlock();
            auto hit = handlers.find(item.second.command);
            if (hit != handlers.end()) {
                hit->second(item.first, item.second);
            }
            lk.lock();
        }
    }

    std::unordered_map<std::string, PeerInfo> m_peers;
    std::unordered_map<std::string, Handler> m_handlers;
    std::queue<std::pair<PeerInfo, Message>> m_queue;
    std::condition_variable m_cv;
    mutable std::mutex m_mutex;
    std::thread m_worker;
    std::atomic<bool> m_running;
};

} // namespace net
