#pragma once

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "../layer1-core/block/block.h"

struct MinerJob {
    BlockHeader header{};
    uint256 target{};
    std::string jobId;
    double difficulty{0.0};
    std::chrono::steady_clock::time_point receivedAt{};
    bool cleanJobs{false};
};

class StratumClient {
public:
    StratumClient(const std::string& url, const std::string& user, const std::string& pass, bool allowRemote);
    void Connect();
    std::optional<MinerJob> AwaitJob();
    void SubmitResult(const MinerJob& job, uint32_t nonce);
    void SendKeepalive();
    double CurrentDifficulty() const { return currentDifficulty_; }
    bool IsConnected() const { return socket_.is_open(); }
    void Reconnect();

private:
    void Subscribe();
    void Authorize();
    bool ReadMessage(boost::property_tree::ptree& out);
    void HandleDifficulty(const boost::property_tree::ptree& msg);
    void HandleSetExtranonce(const boost::property_tree::ptree& msg);
    std::optional<MinerJob> HandleNotify(const boost::property_tree::ptree& msg);
    bool IsStaleJob(const MinerJob& job) const;

    boost::asio::io_context ctx_;
    boost::asio::ip::tcp::socket socket_;
    std::string host_;
    std::string port_;
    std::string user_;
    std::string pass_;
    std::string extranonce1_;
    uint32_t extranonce2Size_{0};
    double currentDifficulty_{1.0};
    std::mutex ioMutex_;
    std::deque<MinerJob> jobQueue_;
    std::mutex jobQueueMutex_;
    std::chrono::steady_clock::time_point lastJobTime_{};
    int reconnectAttempts_{0};
    static constexpr int kMaxReconnectAttempts = 10;
    static constexpr int kJobStaleSeconds = 30;
};

enum class StratumProtocol { V1, V2 };

class StratumPool {
public:
    struct Options {
        std::string url;
        std::string user;
        std::string pass;
        bool allowRemote{false};
        StratumProtocol protocol{StratumProtocol::V1};
        std::function<void(const std::string&)> onSecurityEvent;
    };

    explicit StratumPool(Options opts);
    ~StratumPool();
    void Connect();
    std::optional<MinerJob> AwaitJob();
    void SubmitResult(const MinerJob& job, uint32_t nonce);
    double CurrentDifficulty() const;
    void SendKeepalive();

private:
    void EnsureNonceEntropy();
    void StartHealthMonitor();
    bool MalwareScan();

    Options opts_;
    StratumClient legacyClient_;
    double difficulty_{1.0};
    std::mt19937 nonceRng_;
    std::uniform_int_distribution<uint32_t> nonceDist_;
    std::thread monitorThread_;
    std::atomic<bool> stopMonitor_{false};
};

