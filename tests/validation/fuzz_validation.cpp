#include "../../layer1-core/validation/validation.h"
#include "../../layer1-core/tx/transaction.h"
#include "../../layer1-core/consensus/params.h"
#include <cstdint>
#include <vector>
#include <array>
#include <fstream>
#include <limits>

namespace {

OutPoint NullPrevout()
{
    OutPoint op{};
    op.hash.fill(0);
    op.index = std::numeric_limits<uint32_t>::max();
    return op;
}

Transaction MakeCoinbase(uint64_t value)
{
    Transaction tx;
    tx.vin.resize(1);
    tx.vout.resize(1);
    tx.vin[0].prevout = NullPrevout();
    tx.vin[0].scriptSig = {0x01, 0x02};
    tx.vin[0].sequence = 0xffffffff;
    tx.vout[0].value = value;
    tx.vout[0].scriptPubKey.assign(32, 0x01);
    tx.vin[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    tx.vout[0].assetId = static_cast<uint8_t>(AssetId::TALANTON);
    return tx;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 4) return 0;
    consensus::Params params = consensus::Testnet();
    params.nGenesisBits = 0x207fffff;

    std::vector<uint8_t> buf(data, data + size);
    size_t offset = 0;
    try {
        // Drive transaction deserialization with arbitrary bytes.
        Transaction tx = DeserializeTransaction(buf);
        std::vector<Transaction> blockTxs;
        blockTxs.push_back(MakeCoinbase(consensus::GetBlockSubsidy(1, params, static_cast<uint8_t>(AssetId::TALANTON))));
        blockTxs.push_back(tx);
        (void)ValidateTransactions(blockTxs, params, 1, {});
    } catch (const std::exception&) {
        // ignore parser errors
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) return 0;
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(in)), {});
    LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
    return 0;
}
