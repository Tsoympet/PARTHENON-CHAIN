#pragma once
#include <vector>
#include <cstdint>
#include <string>

struct TxOutPoint {
    std::string txid;     // hex string
    uint32_t index;

    bool operator==(const TxOutPoint& other) const {
        return txid == other.txid && index == other.index;
    }
};

struct TxIn {
    TxOutPoint prevout;
    std::vector<uint8_t> scriptSig;
    uint32_t sequence;

    TxIn() : sequence(0xFFFFFFFF) {}
};
