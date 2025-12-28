#pragma once
#include <vector>
#include <string>
#include "utxo.h"

struct UndoEntry {
    OutPointKey key;
    UTXO utxo;
};

struct BlockUndo {
    std::vector<UndoEntry> spent;
};
