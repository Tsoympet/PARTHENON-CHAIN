#include "keystore.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <fstream>
#include <stdexcept>

namespace wallet {

static void DeriveKey(const std::string& pass, std::array<uint8_t,32>& out)
{
    PKCS5_PBKDF2_HMAC(pass.c_str(), pass.size(), nullptr, 0, 100000, EVP_sha256(), out.size(), out.data());
}

static std::vector<uint8_t> Encrypt(const std::array<uint8_t,32>& key, const std::vector<uint8_t>& plaintext)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> iv(16);
    RAND_bytes(iv.data(), iv.size());

    std::vector<uint8_t> out(iv.begin(), iv.end());
    out.resize(iv.size() + plaintext.size() + 16);

    int len = 0, total = iv.size();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data());
    EVP_EncryptUpdate(ctx, out.data() + total, &len, plaintext.data(), plaintext.size());
    total += len;
    EVP_EncryptFinal_ex(ctx, out.data() + total, &len);
    total += len;
    out.resize(total);
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

static std::vector<uint8_t> Decrypt(const std::array<uint8_t,32>& key, const std::vector<uint8_t>& cipher)
{
    if (cipher.size() < 16) throw std::runtime_error("cipher too small");
    const uint8_t* iv = cipher.data();
    const uint8_t* data = cipher.data() + 16;
    size_t dataLen = cipher.size() - 16;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    std::vector<uint8_t> out(dataLen + 16);
    int len = 0, total = 0;
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv);
    EVP_DecryptUpdate(ctx, out.data(), &len, data, dataLen);
    total += len;
    if (EVP_DecryptFinal_ex(ctx, out.data() + total, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("bad passphrase");
    }
    total += len;
    out.resize(total);
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

void KeyStore::Import(const KeyId& id, const PrivKey& key)
{
    m_keys[id] = key;
}

bool KeyStore::Get(const KeyId& id, PrivKey& out) const
{
    auto it = m_keys.find(id);
    if (it == m_keys.end()) return false;
    out = it->second;
    return true;
}

void KeyStore::EncryptToFile(const std::string& passphrase, const std::string& path) const
{
    std::vector<uint8_t> plain;
    plain.reserve(m_keys.size() * 64);
    for (const auto& kv : m_keys) {
        plain.insert(plain.end(), kv.first.begin(), kv.first.end());
        plain.insert(plain.end(), kv.second.begin(), kv.second.end());
    }
    std::array<uint8_t,32> key;
    DeriveKey(passphrase, key);
    auto enc = Encrypt(key, plain);
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(enc.data()), enc.size());
}

void KeyStore::LoadFromFile(const std::string& passphrase, const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("missing keystore");
    std::vector<uint8_t> enc((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::array<uint8_t,32> key;
    DeriveKey(passphrase, key);
    auto plain = Decrypt(key, enc);
    m_keys.clear();
    size_t offset = 0;
    while (offset + 64 <= plain.size()) {
        KeyId id{}; PrivKey pk{};
        std::copy(plain.begin() + offset, plain.begin() + offset + 32, id.begin());
        offset += 32;
        std::copy(plain.begin() + offset, plain.begin() + offset + 32, pk.begin());
        offset += 32;
        m_keys.emplace(id, pk);
    }
}

size_t KeyStore::ArrayHasher::operator()(const std::array<uint8_t,32>& data) const noexcept
{
    size_t h = 0;
    for (auto b : data) h = (h * 131) ^ b;
    return h;
}

} // namespace wallet
