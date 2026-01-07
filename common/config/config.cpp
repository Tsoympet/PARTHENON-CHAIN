#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

// A lightweight configuration loader for DRACHMA components. This file is
// intentionally self contained so it can be included by utilities or linked
// into services without pulling in unrelated dependencies.
namespace config {

struct NodeConfig {
    std::string network;       // "main" or "testnet"
    std::filesystem::path dataDir;
    std::filesystem::path logFile;
    std::string rpcUser;
    std::string rpcPassword;
    uint16_t p2pPort;
    uint16_t rpcPort;
};

namespace {
std::mutex g_mutex;
std::optional<NodeConfig> g_cached;

std::string Trim(const std::string& in)
{
    auto front = std::find_if_not(in.begin(), in.end(), [](unsigned char c){ return std::isspace(c); });
    auto back  = std::find_if_not(in.rbegin(), in.rend(), [](unsigned char c){ return std::isspace(c); }).base();
    if (front >= back) return {};
    return std::string(front, back);
}

std::unordered_map<std::string, std::string> ParseFile(const std::filesystem::path& path)
{
    std::unordered_map<std::string, std::string> kv;
    std::ifstream in(path);
    if (!in.is_open())
        throw std::runtime_error("Unable to open config file: " + path.string());

    std::string line;
    size_t lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        auto pos = line.find('=');
        if (pos == std::string::npos)
            throw std::runtime_error("Malformed config line " + std::to_string(lineNo));
        std::string key = Trim(line.substr(0, pos));
        std::string val = Trim(line.substr(pos + 1));
        kv[key] = val;
    }
    return kv;
}

uint16_t ToPort(const std::string& s, uint16_t fallback)
{
    try {
        int v = std::stoi(s);
        if (v <= 0 || v > 65535)
            throw std::out_of_range("port range");
        return static_cast<uint16_t>(v);
    } catch (const std::invalid_argument& e) {
        return fallback;
    } catch (const std::out_of_range& e) {
        return fallback;
    }
}

std::filesystem::path DefaultDataDir()
{
    const char* env = std::getenv("DRACHMA_DATA");
    if (env && *env)
        return std::filesystem::path(env);
#ifdef _WIN32
    const char* home = std::getenv("APPDATA");
    if (home)
        return std::filesystem::path(home) / "Drachma";
#else
    const char* home = std::getenv("HOME");
    if (home)
        return std::filesystem::path(home) / ".drachma";
#endif
    return std::filesystem::temp_directory_path() / "drachma";
}

} // namespace

NodeConfig Load(const std::filesystem::path& path)
{
    std::scoped_lock lock(g_mutex);
    if (g_cached)
        return *g_cached;

    NodeConfig cfg{};
    cfg.network = "main";
    cfg.dataDir = DefaultDataDir();
    cfg.logFile = cfg.dataDir / "drachma.log";
    cfg.rpcUser = "drachma";
    cfg.rpcPassword = "change-me";
    cfg.p2pPort = 11311;
    cfg.rpcPort = 8332; // chosen to avoid collision with bitcoin mainnet RPC by default

    if (!path.empty() && std::filesystem::exists(path)) {
        auto kv = ParseFile(path);
        if (auto it = kv.find("network"); it != kv.end())
            cfg.network = it->second;
        if (auto it = kv.find("datadir"); it != kv.end())
            cfg.dataDir = it->second;
        if (auto it = kv.find("logfile"); it != kv.end())
            cfg.logFile = it->second;
        if (auto it = kv.find("rpcuser"); it != kv.end())
            cfg.rpcUser = it->second;
        if (auto it = kv.find("rpcpassword"); it != kv.end())
            cfg.rpcPassword = it->second;
        if (auto it = kv.find("p2pport"); it != kv.end())
            cfg.p2pPort = ToPort(it->second, cfg.p2pPort);
        if (auto it = kv.find("rpcport"); it != kv.end())
            cfg.rpcPort = ToPort(it->second, cfg.rpcPort);
    }

    std::filesystem::create_directories(cfg.dataDir);

    g_cached = cfg;
    return cfg;
}

void Override(const NodeConfig& updated)
{
    std::scoped_lock lock(g_mutex);
    g_cached = updated;
}

} // namespace config
