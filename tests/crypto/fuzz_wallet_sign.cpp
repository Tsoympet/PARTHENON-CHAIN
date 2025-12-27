#include "../../layer2-services/wallet/wallet.h"
#include <cstdint>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 4) return 0;

    wallet::KeyStore store;
    wallet::WalletBackend backend(store);

    wallet::PrivKey priv{};
    for (size_t i = 0; i < priv.size() && i < size; ++i) priv[i] = data[i];
    auto id = backend.ImportKey(priv);

    OutPoint op{};
    TxOut utxo{};
    utxo.value = 1'000'000 + (data[0] % 10'000);
    utxo.scriptPubKey.assign(32, 0x01);
    backend.AddUTXO(op, utxo);

    TxOut out{};
    out.value = utxo.value > 1 ? utxo.value - 1 : 1;
    out.scriptPubKey.assign(32, 0x02);

    try {
        backend.CreateSpend({out}, id, 1);
    } catch (...) {
        // Best-effort fuzzing; creation may throw on malformed inputs.
    }

    return 0;
}
