#pragma once

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "../layer1-core/block/block.h"

struct MinerJob {
    BlockHeader header{};
    uint256 target{};
    std::string jobId;
    double difficulty{0.0};
};

class StratumClient {
public:
    StratumClient(const std::string& url, const std::string& user, const std::string& pass, bool allowRemote);
    void Connect();
    std::optional<MinerJob> AwaitJob();
    void SubmitResult(const MinerJob& job, uint32_t nonce);
    void SendKeepalive();
    double CurrentDifficulty() const { return currentDifficulty_; }

private:
    void Subscribe();
    void Authorize();
    bool ReadMessage(boost::property_tree::ptree& out);
    void HandleDifficulty(const boost::property_tree::ptree& msg);
    std::optional<MinerJob> HandleNotify(const boost::property_tree::ptree& msg);

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
};

