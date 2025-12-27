#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace wallet {

using KeyId = std::array<uint8_t,32>;
using PrivKey = std::array<uint8_t,32>;
using PubKey  = std::array<uint8_t,33>;

class KeyStore {
public:
    KeyStore() = default;

    void Import(const KeyId& id, const PrivKey& key);
    bool Get(const KeyId& id, PrivKey& out) const;
    void EncryptToFile(const std::string& passphrase, const std::string& path) const;
    void LoadFromFile(const std::string& passphrase, const std::string& path);

private:
    struct ArrayHasher {
        size_t operator()(const std::array<uint8_t,32>& data) const noexcept;
    };

    std::unordered_map<KeyId, PrivKey, ArrayHasher> m_keys;
};

} // namespace wallet
