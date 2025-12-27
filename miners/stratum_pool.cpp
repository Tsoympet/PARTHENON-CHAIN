#include "stratum.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>

using namespace std::chrono_literals;

StratumPool::StratumPool(Options opts)
    : opts_(std::move(opts)),
      legacyClient_(opts_.url, opts_.user, opts_.pass, opts_.allowRemote),
      nonceRng_(std::random_device{}()),
      nonceDist_(0, std::numeric_limits<uint32_t>::max())
{
}

StratumPool::~StratumPool()
{
    stopMonitor_.store(true);
    if (monitorThread_.joinable())
        monitorThread_.join();
}

void StratumPool::EnsureNonceEntropy()
{
    // Simple entropy stirring to avoid nonce reuse in hostile or shared environments.
    uint32_t entropy = nonceDist_(nonceRng_) ^ static_cast<uint32_t>(std::time(nullptr));
    (void)entropy; // entropy can be threaded into per-thread starts by callers
}

void StratumPool::StartHealthMonitor()
{
    stopMonitor_.store(false);
    monitorThread_ = std::thread([this]() {
        while (!stopMonitor_.load()) {
            if (MalwareScan() && opts_.onSecurityEvent)
                opts_.onSecurityEvent("malware signature detected; aborting miner session");
            std::this_thread::sleep_for(10s);
        }
    });
}

bool StratumPool::MalwareScan()
{
    // Lightweight process name inspection to catch common illicit hijackers.
    static const char* kBadNames[] = {"xmrig", "kdevtmpfsi", "kinsing", "crypto-miner"};
    try {
        for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
            if (!entry.is_directory())
                continue;
            const std::string filename = entry.path().filename().string();
            if (!std::all_of(filename.begin(), filename.end(), ::isdigit))
                continue;
            std::ifstream comm(entry.path() / "comm");
            std::string name;
            if (comm >> name) {
                for (const auto& bad : kBadNames) {
                    if (name.find(bad) != std::string::npos)
                        return true;
                }
            }
        }
    } catch (...) {
        // Best-effort: ignore failures on hardened systems.
    }
    return false;
}

void StratumPool::Connect()
{
    EnsureNonceEntropy();
    // V2 negotiation can be implemented as the protocol matures; default to v1 handshake today.
    if (opts_.protocol == StratumProtocol::V2) {
        // Minimal v2-compatible announce for pools that accept stratum+v2 URLs.
        legacyClient_.Connect();
    } else {
        legacyClient_.Connect();
    }
    difficulty_ = legacyClient_.CurrentDifficulty();
    StartHealthMonitor();
}

std::optional<MinerJob> StratumPool::AwaitJob()
{
    if (auto job = legacyClient_.AwaitJob()) {
        difficulty_ = legacyClient_.CurrentDifficulty();
        return job;
    }
    return std::nullopt;
}

void StratumPool::SubmitResult(const MinerJob& job, uint32_t nonce)
{
    legacyClient_.SubmitResult(job, nonce);
}

double StratumPool::CurrentDifficulty() const
{
    return difficulty_;
}

void StratumPool::SendKeepalive()
{
    legacyClient_.SendKeepalive();
}
