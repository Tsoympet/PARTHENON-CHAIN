#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct TxOut {
    uint64_t value;
    std::string scriptPubKey;
};

struct Transaction {
    std::vector<TxOut> vout;
};
