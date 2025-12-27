#include <unordered_map>
#include <string>
#include <vector>
#include "../messages/crosschain_msg.cpp"
#include "../validation/proof_validator.cpp"

namespace crosschain {

class BridgeManager {
public:
    void RegisterChain(const std::string& name, const std::array<uint8_t,32>& genesis)
    {
        m_chainTips[name] = genesis;
    }

    bool VerifyAndStore(const std::string& name, const std::vector<HeaderProof>& proofs)
    {
        auto it = m_chainTips.find(name);
        if (it == m_chainTips.end()) return false;
        ProofValidator validator;
        if (!validator.ValidateChain(proofs, it->second))
            return false;
        // update tip to newest proof hash
        std::array<uint8_t,32> newTip{};
        if (!proofs.empty()) {
            // last hash equals expected tip when valid
            SHA256_CTX ctx;
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, proofs.back().header.data(), proofs.back().header.size());
            SHA256_Final(newTip.data(), &ctx);
            SHA256_Init(&ctx);
            SHA256_Update(&ctx, newTip.data(), newTip.size());
            SHA256_Final(newTip.data(), &ctx);
        }
        m_chainTips[name] = newTip;
        return true;
    }

    void RelayMessage(const std::string& destChain, const CrossChainMessage& msg)
    {
        m_messages[destChain].push_back(msg);
    }

    std::vector<CrossChainMessage> PendingFor(const std::string& chain) const
    {
        auto it = m_messages.find(chain);
        if (it == m_messages.end()) return {};
        return it->second;
    }

private:
    std::unordered_map<std::string, std::array<uint8_t,32>> m_chainTips;
    std::unordered_map<std::string, std::vector<CrossChainMessage>> m_messages;
};

} // namespace crosschain
