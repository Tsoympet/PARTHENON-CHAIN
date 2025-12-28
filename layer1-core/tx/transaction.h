#pragma once
#include "txin.h"
#include "txout.h"
#include <vector>
#include <string>
#include <cstdint>

class Transaction {
public:
    uint32_t version;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    uint32_t lockTime;

    Transaction();

    bool isCoinbase() const;
    uint64_t totalOutput() const;
    std::string getTxID() const;

    std::vector<uint8_t> serialize() const;
    static Transaction deserialize(const std::vector<uint8_t>& data);
};
