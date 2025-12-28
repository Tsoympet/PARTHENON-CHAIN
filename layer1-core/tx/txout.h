#pragma once
#include <vector>
#include <cstdint>

struct TxOut {
    uint64_t value;                 // smallest unit
    std::vector<uint8_t> scriptPubKey;

    TxOut() : value(0) {}
    TxOut(uint64_t v, const std::vector<uint8_t>& script)
        : value(v), scriptPubKey(script) {}
};
