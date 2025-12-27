#include "../crypto/schnorr.h"
#include "../tx/transaction.h"
#include <stdexcept>
#include <array>

// Minimal script: scriptPubKey encodes a 32-byte x-only public key.
// scriptSig encodes a 64-byte Schnorr signature over the transaction hash.

bool VerifyScript(const Transaction& tx, size_t inputIndex)
{
    if (inputIndex >= tx.vin.size())
        throw std::runtime_error("input index out of range");

    const TxIn& in = tx.vin[inputIndex];
    if (in.scriptSig.size() != 64)
        return false;

    if (in.prevout.index >= tx.vout.size()) {
        // For validation we only require scriptPubKey at referenced UTXO; here assume provided in scriptSig? handled upstream.
    }

    // scriptPubKey must exist in the spending transaction's vin metadata? In this model, TxIn carries no pubkey; expect vout to supply.
    // The referenced output script must be supplied externally by caller. For deterministic validation we expect scriptSig || scriptPubKey pair.
    // To keep interpreter self-contained, treat scriptSig as signature and scriptPubKey embedded in the referenced output of the same transaction.

    std::array<uint8_t,64> sig{};
    std::copy(in.scriptSig.begin(), in.scriptSig.end(), sig.begin());

    // For this minimal model, we require the output being spent to exist in tx.vout? In practice comes from UTXO set; caller must provide.
    // Interpreter only validates signature using provided pubkey bytes.
    if (in.prevout.index >= tx.vout.size())
        return false;
    const TxOut& prevout = tx.vout[in.prevout.index];
    if (prevout.scriptPubKey.size() != 32)
        return false;

    std::array<uint8_t,32> pubkey{};
    std::copy(prevout.scriptPubKey.begin(), prevout.scriptPubKey.end(), pubkey.begin());

    auto msg = Serialize(tx);
    return VerifySchnorr(pubkey, sig, msg);
}
