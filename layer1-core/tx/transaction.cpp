#include "transaction.h"
#include <cstring>
#include <stdexcept>

static void WriteUint32(std::vector<uint8_t>& out, uint32_t v)
{
    for (int i = 0; i < 4; ++i)
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

static uint32_t ReadUint32(const std::vector<uint8_t>& data, size_t& offset)
{
    if (offset + 4 > data.size()) throw std::runtime_error("deserialize uint32 overflow");
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i)
        v |= static_cast<uint32_t>(data[offset++]) << (8 * i);
    return v;
}

static void WriteUint64(std::vector<uint8_t>& out, uint64_t v)
{
    for (int i = 0; i < 8; ++i)
        out.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
}

static uint64_t ReadUint64(const std::vector<uint8_t>& data, size_t& offset)
{
    if (offset + 8 > data.size()) throw std::runtime_error("deserialize uint64 overflow");
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i)
        v |= static_cast<uint64_t>(data[offset++]) << (8 * i);
    return v;
}

static void WriteVarBytes(std::vector<uint8_t>& out, const std::vector<uint8_t>& bytes)
{
    WriteUint32(out, static_cast<uint32_t>(bytes.size()));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

static std::vector<uint8_t> ReadVarBytes(const std::vector<uint8_t>& data, size_t& offset)
{
    uint32_t len = ReadUint32(data, offset);
    if (offset + len > data.size()) throw std::runtime_error("deserialize var bytes overflow");
    std::vector<uint8_t> out(data.begin() + offset, data.begin() + offset + len);
    offset += len;
    return out;
}

std::vector<uint8_t> Serialize(const Transaction& tx)
{
    std::vector<uint8_t> out;
    WriteUint32(out, tx.version);
    WriteUint32(out, static_cast<uint32_t>(tx.vin.size()));
    for (const auto& in : tx.vin) {
        out.insert(out.end(), in.prevout.hash.begin(), in.prevout.hash.end());
        WriteUint32(out, in.prevout.index);
        WriteVarBytes(out, in.scriptSig);
        WriteUint32(out, in.sequence);
    }
    WriteUint32(out, static_cast<uint32_t>(tx.vout.size()));
    for (const auto& o : tx.vout) {
        WriteUint64(out, o.value);
        WriteVarBytes(out, o.scriptPubKey);
    }
    WriteUint32(out, tx.lockTime);
    return out;
}

Transaction DeserializeTransaction(const std::vector<uint8_t>& data)
{
    Transaction tx;
    size_t offset = 0;
    tx.version = ReadUint32(data, offset);
    uint32_t vinSize = ReadUint32(data, offset);
    tx.vin.resize(vinSize);
    for (uint32_t i = 0; i < vinSize; ++i) {
        if (offset + tx.vin[i].prevout.hash.size() > data.size())
            throw std::runtime_error("deserialize hash overflow");
        std::memcpy(tx.vin[i].prevout.hash.data(), data.data() + offset, tx.vin[i].prevout.hash.size());
        offset += tx.vin[i].prevout.hash.size();
        tx.vin[i].prevout.index = ReadUint32(data, offset);
        tx.vin[i].scriptSig = ReadVarBytes(data, offset);
        tx.vin[i].sequence = ReadUint32(data, offset);
    }
    uint32_t voutSize = ReadUint32(data, offset);
    tx.vout.resize(voutSize);
    for (uint32_t i = 0; i < voutSize; ++i) {
        tx.vout[i].value = ReadUint64(data, offset);
        tx.vout[i].scriptPubKey = ReadVarBytes(data, offset);
    }
    tx.lockTime = ReadUint32(data, offset);
    if (offset != data.size())
        throw std::runtime_error("unexpected trailing data");
    return tx;
}

uint256 TransactionHash(const Transaction& tx)
{
    auto bytes = Serialize(tx);
    return TaggedHash("TX", bytes.data(), bytes.size());
}

uint256 Transaction::GetHash() const
{
    return TransactionHash(*this);
}
