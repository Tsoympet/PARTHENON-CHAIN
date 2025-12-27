#pragma once
#include "../tx/transaction.h"

// Validate an input's signature against the provided UTXO's scriptPubKey.
// This overload requires the caller to supply the previous output being spent
// to avoid assuming the input references an output within the same transaction.
bool VerifyScript(const Transaction& tx, size_t inputIndex, const TxOut& utxo);
