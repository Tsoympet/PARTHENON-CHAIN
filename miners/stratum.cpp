#include "stratum.h"

#include <boost/property_tree/json_parser.hpp>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

using boost::asio::ip::tcp;

namespace {
std::string ExtractHost(const std::string& url)
{
    auto pos = url.find("://");
    std::string hostport = pos == std::string::npos ? url : url.substr(pos + 3);
    auto colon = hostport.find(":");
    if (colon == std::string::npos)
        throw std::invalid_argument("stratum url must include host:port");
    return hostport.substr(0, colon);
}

std::string ExtractPort(const std::string& url)
{
    auto pos = url.find("://");
    std::string hostport = pos == std::string::npos ? url : url.substr(pos + 3);
    auto colon = hostport.find(":");
    if (colon == std::string::npos)
        throw std::invalid_argument("stratum url must include host:port");
    return hostport.substr(colon + 1);
}

uint32_t ParseLE32(const std::string& hex, size_t offset)
{
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        auto byte = static_cast<uint32_t>(std::stoul(hex.substr((offset + i) * 2, 2), nullptr, 16));
        out |= byte << (8 * i);
    }
    return out;
}

uint256 FromHex(const std::string& hex)
{
    if (hex.size() % 2 != 0)
        throw std::invalid_argument("expected even-length hex string");
    uint256 out{};
    size_t bytes = hex.size() / 2;
    if (bytes > out.size())
        throw std::invalid_argument("hex string too long for uint256");
    size_t offset = out.size() - bytes;
    for (size_t i = 0; i < bytes; ++i) {
        auto byte = std::stoul(hex.substr(i * 2, 2), nullptr, 16);
        out[offset + i] = static_cast<uint8_t>(byte);
    }
    return out;
}

MinerJob ParseHeaderJob(const std::string& headerHex, const std::string& targetHex, const std::string& jobId)
{
    if (headerHex.size() != 160)
        throw std::invalid_argument("expected 80-byte header hex");
    MinerJob job{};
    job.header.version = ParseLE32(headerHex, 0);
    job.header.prevBlockHash = FromHex(headerHex.substr(8, 64));
    job.header.merkleRoot = FromHex(headerHex.substr(72, 64));
    job.header.time = ParseLE32(headerHex, 136 / 2);
    job.header.bits = ParseLE32(headerHex, 144 / 2);
    job.header.nonce = ParseLE32(headerHex, 152 / 2);
    job.target = targetHex.empty() ? uint256{} : FromHex(targetHex);
    job.jobId = jobId;
    return job;
}

} // namespace

StratumClient::StratumClient(const std::string& url, const std::string& user, const std::string& pass, bool allowRemote)
    : ctx_(), socket_(ctx_), host_(ExtractHost(url)), port_(ExtractPort(url)), user_(user), pass_(pass)
{
    if (!allowRemote && host_ != "127.0.0.1" && host_ != "localhost")
        throw std::runtime_error("remote stratum connections require --allow-remote");
}

void StratumClient::Connect()
{
    tcp::resolver resolver(ctx_);
    auto endpoints = resolver.resolve(host_, port_);
    boost::asio::connect(socket_, endpoints);
    Subscribe();
    Authorize();
}

bool StratumClient::ReadMessage(boost::property_tree::ptree& out)
{
    boost::asio::streambuf buf;
    boost::asio::read_until(socket_, buf, '\n');
    std::istream is(&buf);
    try {
        boost::property_tree::read_json(is, out);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse stratum JSON: " << e.what() << "\n";
        return false;
    }
}

std::optional<MinerJob> StratumClient::AwaitJob()
{
    boost::property_tree::ptree msg;
    if (!ReadMessage(msg))
        return std::nullopt;
    auto method = msg.get<std::string>("method", "");
    if (method == "mining.notify") {
        return HandleNotify(msg);
    } else if (method == "mining.set_difficulty") {
        HandleDifficulty(msg);
        return std::nullopt;
    }
    return std::nullopt;
}

void StratumClient::SubmitResult(const MinerJob& job, uint32_t nonce)
{
    std::lock_guard<std::mutex> g(ioMutex_);
    std::ostringstream payload;
    payload << "{\"id\":4,\"method\":\"mining.submit\",\"params\":[\"" << user_ << "\",\"" << job.jobId
            << "\",\"" << std::hex << nonce << "\"]}\n";
    boost::asio::write(socket_, boost::asio::buffer(payload.str()));
}

void StratumClient::SendKeepalive()
{
    std::lock_guard<std::mutex> g(ioMutex_);
    static const std::string ping = "{\"id\":100,\"method\":\"mining.ping\",\"params\":[]}\n";
    boost::asio::write(socket_, boost::asio::buffer(ping));
}

void StratumClient::Subscribe()
{
    std::lock_guard<std::mutex> g(ioMutex_);
    static const std::string subscribe = "{\"id\":1,\"method\":\"mining.subscribe\",\"params\":[]}\n";
    boost::asio::write(socket_, boost::asio::buffer(subscribe));
}

void StratumClient::Authorize()
{
    std::lock_guard<std::mutex> g(ioMutex_);
    std::ostringstream auth;
    auth << "{\"id\":2,\"method\":\"mining.authorize\",\"params\":[\"" << user_ << "\",\"" << pass_ << "\"]}\n";
    boost::asio::write(socket_, boost::asio::buffer(auth.str()));
}

void StratumClient::HandleDifficulty(const boost::property_tree::ptree& msg)
{
    try {
        auto params = msg.get_child("params");
        currentDifficulty_ = params.front().second.get_value<double>();
    } catch (...) {
        currentDifficulty_ = 1.0;
    }
}

std::optional<MinerJob> StratumClient::HandleNotify(const boost::property_tree::ptree& msg)
{
    try {
        auto params = msg.get_child("params");
        std::vector<std::string> fields;
        for (const auto& p : params)
            fields.push_back(p.second.get_value<std::string>());
        if (fields.size() < 3)
            return std::nullopt;
        const std::string& jobId = fields[0];
        const std::string& headerHex = fields[1];
        const std::string& targetHex = fields[2];
        MinerJob job = ParseHeaderJob(headerHex, targetHex, jobId);
        job.difficulty = currentDifficulty_;
        return job;
    } catch (const std::exception& e) {
        std::cerr << "stratum notify parse error: " << e.what() << "\n";
        return std::nullopt;
    }
}

StratumPool::StratumPool(Options opts)
    : opts_(std::move(opts)),
      legacyClient_(opts_.url, opts_.user, opts_.pass, opts_.allowRemote)
{
}

StratumPool::~StratumPool() = default;

void StratumPool::Connect()
{
    legacyClient_.Connect();
    difficulty_ = legacyClient_.CurrentDifficulty();
}

std::optional<MinerJob> StratumPool::AwaitJob()
{
    return legacyClient_.AwaitJob();
}

void StratumPool::SubmitResult(const MinerJob& job, uint32_t nonce)
{
    legacyClient_.SubmitResult(job, nonce);
}

double StratumPool::CurrentDifficulty() const
{
    return legacyClient_.CurrentDifficulty();
}

void StratumPool::SendKeepalive()
{
    legacyClient_.SendKeepalive();
}

