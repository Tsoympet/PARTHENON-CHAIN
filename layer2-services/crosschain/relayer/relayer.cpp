#include <vector>
#include <string>
#include "../bridge/bridge_manager.cpp"

namespace crosschain {

class Relayer {
public:
    explicit Relayer(BridgeManager& bridge) : m_bridge(bridge) {}

    bool SubmitProof(const std::string& chain, const std::vector<HeaderProof>& proofs)
    {
        return m_bridge.VerifyAndStore(chain, proofs);
    }

    void Forward(const std::string& chain, const CrossChainMessage& msg)
    {
        m_bridge.RelayMessage(chain, msg);
    }

private:
    BridgeManager& m_bridge;
};

} // namespace crosschain
