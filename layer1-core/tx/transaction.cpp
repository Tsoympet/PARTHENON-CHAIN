#include "transaction.h"
#include "serialization.h"
#include "../crypto/sha256.h"
#include <sstream>
#include <iomanip>

Transaction::Transaction() : version(1), lockTime(0) {}

bool Transaction::isCoinbase() const {
    return vin.size() == 1 && vin[0].prevout.txid.empty() && vin[0].prevout.index == 0xFFFFFFFF;
}

uint64_t Transaction::totalOutput() const {
    uint64_t sum = 0;
    for (const auto& o : vout) sum += o.value;
    return sum;
}

static std::string hexStr(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    for (auto b : data)
        oss << std::hex << std::setw(2) << std::setfill('0') << int(b);
    return oss.str();
}

std::string Transaction::getTxID() const {
    auto raw = serialize();
    auto hash = sha256d(raw);   // double sha256 from existing crypto module
    return hexStr(hash);
}

std::vector<uint8_t> Transaction::serialize() const {
    std::vector<uint8_t> buf;
    Serializer::writeUint32(buf, version);

    Serializer::writeUint32(buf, static_cast<uint32_t>(vin.size()));
    for (const auto& in : vin) {
        Serializer::writeVarBytes(buf, std::vector<uint8_t>(in.prevout.txid.begin(), in.prevout.txid.end()));
        Serializer::writeUint32(buf, in.prevout.index);
        Serializer::writeVarBytes(buf, in.scriptSig);
        Serializer::writeUint32(buf, in.sequence);
    }

    Serializer::writeUint32(buf, static_cast<uint32_t>(vout.size()));
    for (const auto& out : vout) {
        Serializer::writeUint64(buf, out.value);
        Serializer::writeVarBytes(buf, out.scriptPubKey);
    }

    Serializer::writeUint32(buf, lockTime);
    return buf;
}

Transaction Transaction::deserialize(const std::vector<uint8_t>& data) {
    size_t off = 0;
    Transaction tx;

    tx.version = Serializer::readUint32(data, off);

    uint32_t vinCount = Serializer::readUint32(data, off);
    for (uint32_t i = 0; i < vinCount; ++i) {
        TxIn in;
        auto txidBytes = Serializer::readVarBytes(data, off);
        in.prevout.txid = std::string(txidBytes.begin(), txidBytes.end());
        in.prevout.index = Serializer::readUint32(data, off);
        in.scriptSig = Serializer::readVarBytes(data, off);
        in.sequence = Serializer::readUint32(data, off);
        tx.vin.push_back(in);
    }

    uint32_t voutCount = Serializer::readUint32(data, off);
    for (uint32_t i = 0; i < voutCount; ++i) {
        TxOut out;
        out.value = Serializer::readUint64(data, off);
        out.scriptPubKey = Serializer::readVarBytes(data, off);
        tx.vout.push_back(out);
    }

    tx.lockTime = Serializer::readUint32(data, off);
    return tx;
}
