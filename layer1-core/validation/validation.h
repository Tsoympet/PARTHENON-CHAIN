#pragma once
#include "../block/block.h"
#include "../consensus/params.h"
#include <functional>
#include <optional>

using UTXOLookup = std::function<std::optional<TxOut>(const OutPoint&)>;

bool ValidateBlockHeader(const BlockHeader& header, const consensus::Params& params);
bool ValidateTransactions(const std::vector<Transaction>& txs, const consensus::Params& params, int height, const UTXOLookup& lookup = {});
bool ValidateBlock(const Block& block, const consensus::Params& params, int height, const UTXOLookup& lookup = {});
