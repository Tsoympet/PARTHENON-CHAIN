#include "bridge_manager.h"

#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>

namespace crosschain {
namespace {
std::string Hex(const std::vector<uint8_t>& data)
{
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(data.size() * 2);
    for (auto b : data) {
        out.push_back(hex[b >> 4]);
        out.push_back(hex[b & 0x0f]);
    }
    return out;
}

std::vector<uint8_t> FromHex(const std::string& h)
{
    std::vector<uint8_t> out;
    out.reserve(h.size() / 2);
    for (size_t i = 0; i + 1 < h.size(); i += 2) {
        uint8_t hi = static_cast<uint8_t>(std::stoi(h.substr(i, 2), nullptr, 16));
        out.push_back(hi);
    }
    return out;
}

std::array<uint8_t, 32> HashVec(const std::vector<uint8_t>& data)
{
    std::array<uint8_t, 32> out{};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final(out.data(), &ctx);
    return out;
}

} // namespace

BridgeManager::BridgeManager(const std::string& dbPath)
{
    leveldb::Options opts;
    opts.create_if_missing = true;
    leveldb::DB* raw{nullptr};
    auto status = leveldb::DB::Open(opts, dbPath, &raw);
    if (!status.ok()) {
        throw std::runtime_error("failed opening bridge db: " + status.ToString());
    }
    m_db.reset(raw);
}

bool BridgeManager::RegisterChain(const std::string& name, const ChainConfig& config)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_chains[name] = config;
    return true;
}

BridgeLock BridgeManager::InitiateOutboundLock(const std::string& chain,
                                               const std::string& txid,
                                               const std::string& destination,
                                               uint64_t amount,
                                               const std::array<uint8_t, 32>& secretHash,
                                               uint64_t timeoutHeight,
                                               const std::array<uint8_t, 32>& coordinatorPrivKey,
                                               std::vector<uint8_t>& signatureOut)
{
    if (!m_chains.count(chain)) {
        throw std::runtime_error("unknown chain: " + chain);
    }

    BridgeLock lock{};
    lock.id = NewLockId();
    lock.chain = chain;
    lock.txid = txid;
    lock.destination = destination;
    lock.amount = amount;
    lock.secretHash = secretHash;
    lock.timeoutHeight = timeoutHeight;

    auto serialized = SerializeLock(lock);
    auto msgHash = HashVec(std::vector<uint8_t>(serialized.begin(), serialized.end()));
    auto sig = SignMessage(coordinatorPrivKey, std::vector<uint8_t>(msgHash.begin(), msgHash.end()));
    signatureOut.assign(sig.begin(), sig.end());

    PersistLock(lock);
    return lock;
}

bool BridgeManager::DetectInboundLock(const std::string& chain,
                                      const std::vector<HeaderProof>& proofs,
                                      const BridgeLock& observedLock)
{
    auto it = m_chains.find(chain);
    if (it == m_chains.end()) return false;

    ProofValidator validator;
    if (!validator.ValidateChain(proofs, it->second.genesisHash)) return false;

    BridgeLock lock = observedLock;
    lock.id = NewLockId();
    lock.inbound = true;
    PersistLock(lock);

    CrossChainMessage msg{};
    msg.source = chain;
    msg.destination = "drachma";
    auto raw = SerializeLock(lock);
    msg.payload.assign(raw.begin(), raw.end());
    RelayMessage("drachma", msg);
    return true;
}

bool BridgeManager::Claim(const std::string& lockId,
                          const std::vector<uint8_t>& secret,
                          uint64_t currentHeight,
                          std::vector<uint8_t>& signatureOut)
{
    auto existing = GetLock(lockId);
    if (!existing) return false;
    auto lock = *existing;
    if (lock.claimed || lock.refunded) return false;
    if (currentHeight >= lock.timeoutHeight) return false;

    auto hash = HashVec(secret);
    if (hash != lock.secretHash) return false;

    lock.claimed = true;
    lock.secret = secret;
    PersistLock(lock);

    auto sig = SignMessage(lock.secretHash, secret); // deterministically derive signing key from hash
    signatureOut.assign(sig.begin(), sig.end());
    return true;
}

bool BridgeManager::Refund(const std::string& lockId, uint64_t currentHeight)
{
    auto existing = GetLock(lockId);
    if (!existing) return false;
    auto lock = *existing;
    if (lock.refunded) return false;
    if (currentHeight < lock.timeoutHeight) return false;

    lock.refunded = true;
    PersistLock(lock);
    return true;
}

std::optional<BridgeLock> BridgeManager::GetLock(const std::string& lockId) const
{
    std::lock_guard<std::mutex> g(m_mutex);
    std::string val;
    auto status = m_db->Get(leveldb::ReadOptions{}, lockId, &val);
    if (!status.ok()) return std::nullopt;
    return DeserializeLock(val);
}

std::vector<CrossChainMessage> BridgeManager::PendingFor(const std::string& chain) const
{
    auto it = m_messages.find(chain);
    if (it == m_messages.end()) return {};
    return it->second;
}

void BridgeManager::RelayMessage(const std::string& destChain, const CrossChainMessage& msg)
{
    std::lock_guard<std::mutex> g(m_mutex);
    m_messages[destChain].push_back(msg);
}

std::array<uint8_t, 64> BridgeManager::SignMessage(const std::array<uint8_t, 32>& privKey,
                                                   const std::vector<uint8_t>& message) const
{
    std::array<uint8_t, 32> msgHash = HashVec(message);
    std::array<uint8_t, 64> sig{};
    if (!schnorr_sign(privKey.data(), msgHash.data(), sig.data())) {
        throw std::runtime_error("schnorr sign failed");
    }
    return sig;
}

std::string BridgeManager::SerializeLock(const BridgeLock& lock) const
{
    std::ostringstream ss;
    ss << lock.id << '|' << lock.chain << '|' << lock.txid << '|' << lock.destination << '|' << lock.amount << '|';
    ss << Hex(std::vector<uint8_t>(lock.secretHash.begin(), lock.secretHash.end())) << '|';
    ss << Hex(lock.secret) << '|' << lock.timeoutHeight << '|' << lock.inbound << '|' << lock.claimed << '|' << lock.refunded;
    return ss.str();
}

BridgeLock BridgeManager::DeserializeLock(const std::string& raw) const
{
    BridgeLock lock{};
    std::stringstream ss(raw);
    std::string token;
    std::getline(ss, lock.id, '|');
    std::getline(ss, lock.chain, '|');
    std::getline(ss, lock.txid, '|');
    std::getline(ss, lock.destination, '|');
    std::getline(ss, token, '|');
    lock.amount = std::stoull(token);
    std::getline(ss, token, '|');
    auto h = FromHex(token);
    std::copy(h.begin(), h.end(), lock.secretHash.begin());
    std::getline(ss, token, '|');
    lock.secret = FromHex(token);
    std::getline(ss, token, '|');
    lock.timeoutHeight = std::stoull(token);
    std::getline(ss, token, '|');
    lock.inbound = token == "1";
    std::getline(ss, token, '|');
    lock.claimed = token == "1";
    std::getline(ss, token, '|');
    lock.refunded = token == "1";
    return lock;
}

std::string BridgeManager::NewLockId()
{
    static uint64_t counter{0};
    std::array<uint8_t, 32> entropy{};
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, &counter, sizeof(counter));
    SHA256_Final(entropy.data(), &ctx);
    ++counter;
    return Hex(std::vector<uint8_t>(entropy.begin(), entropy.end()));
}

void BridgeManager::PersistLock(const BridgeLock& lock)
{
    std::lock_guard<std::mutex> g(m_mutex);
    auto status = m_db->Put(leveldb::WriteOptions{}, lock.id, SerializeLock(lock));
    if (!status.ok()) {
        throw std::runtime_error("bridge lock persistence failed: " + status.ToString());
    }
}

} // namespace crosschain

