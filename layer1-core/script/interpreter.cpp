#include "interpreter.h"
#include "../crypto/schnorr.h"
#include "../tx/transaction.h"
#include <stdexcept>
#include <array>
#include <algorithm>

// Minimal script: scriptPubKey encodes a 32-byte x-only public key.
// scriptSig encodes a 64-byte Schnorr signature over the transaction hash.

bool VerifyScript(const Transaction& tx, size_t inputIndex, const TxOut& utxo)
{
    if (inputIndex >= tx.vin.size())
        throw std::runtime_error("input index out of range");

    const TxIn& in = tx.vin[inputIndex];
    if (in.scriptSig.size() != 64)
        return false;

    if (utxo.scriptPubKey.size() != 32)
        return false;

    std::array<uint8_t,64> sig{};
    std::copy(in.scriptSig.begin(), in.scriptSig.end(), sig.begin());

    std::array<uint8_t,32> pubkey{};
    std::copy(utxo.scriptPubKey.begin(), utxo.scriptPubKey.end(), pubkey.begin());

    auto msg = Serialize(tx);
    return VerifySchnorr(pubkey, sig, msg);
}
