#include <boost/asio.hpp>
#include <csignal>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "consensus/params.h"
#include "validation/validation.h"
#include "../layer2-services/policy/policy.h"
#include "../layer2-services/mempool/mempool.h"
#include "../layer2-services/net/p2p.h"
#include "../layer2-services/rpc/rpcserver.h"
#include "../layer2-services/index/txindex.h"
#include "../layer2-services/wallet/wallet.h"
#include "../sidechain/wasm/runtime/engine.h"
#include "../sidechain/state/state_store.h"
#include "../sidechain/rpc/wasm_rpc.h"

namespace {

std::string DefaultDataDir()
{
    const char* home = std::getenv("HOME");
    if (!home) home = std::getenv("USERPROFILE");
    std::filesystem::path base;
    if (home) {
        base = std::filesystem::path(home);
    } else {
        const char* drive = std::getenv("HOMEDRIVE");
        const char* path = std::getenv("HOMEPATH");
        if (drive && path) base = std::filesystem::path(std::string(drive) + path);
        else base = std::filesystem::path(".");
    }
    return (base / ".drachma").string();
}

struct Config {
    std::string network{"mainnet"};
    std::string datadir{DefaultDataDir()};
    std::string rpcuser{"user"};
    std::string rpcpassword{"pass"};
    uint16_t rpcport{8332};
    uint16_t p2pport{9333};
    bool listen{true};
};

Config ParseArgs(int argc, char* argv[])
{
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        auto takeValue = [&](const std::string& prefix, auto& out) {
            if (arg.rfind(prefix, 0) == 0 && arg.size() > prefix.size()) {
                using T = std::remove_reference_t<decltype(out)>;
                try {
                    out = static_cast<T>(std::stoul(arg.substr(prefix.size())));
                    return true;
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid numeric value for " << prefix << ": " << e.what() << "\n";
                    return false;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Numeric value out of range for " << prefix << ": " << e.what() << "\n";
                    return false;
                }
            }
            return false;
        };
        if (arg.rfind("--datadir=", 0) == 0) cfg.datadir = arg.substr(10);
        else if (arg.rfind("--network=", 0) == 0) cfg.network = arg.substr(10);
        else if (arg.rfind("--rpcuser=", 0) == 0) cfg.rpcuser = arg.substr(10);
        else if (arg.rfind("--rpcpassword=", 0) == 0) cfg.rpcpassword = arg.substr(14);
        else if (takeValue("--rpcport=", cfg.rpcport)) {}
        else if (takeValue("--port=", cfg.p2pport)) {}
        else if (arg == "--nolisten") cfg.listen = false;
    }
    return cfg;
}

const consensus::Params& ParamsFor(const std::string& network)
{
    if (network == "testnet" || network == "regtest")
        return consensus::Testnet();
    return consensus::Main();
}

void EnsureDatadir(const std::string& path)
{
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    (void)ec;
}

std::vector<uint8_t> SeedFromPath(const std::string& path)
{
    std::vector<uint8_t> seed;
    seed.reserve(32);
    for (size_t i = 0; i < path.size() && seed.size() < 32; ++i)
        seed.push_back(static_cast<uint8_t>(path[i]));
    while (seed.size() < 32) seed.push_back(static_cast<uint8_t>(seed.size()));
    return seed;
}

} // namespace

int main(int argc, char* argv[])
{
    Config cfg = ParseArgs(argc, argv);
    EnsureDatadir(cfg.datadir);

    const auto& params = ParamsFor(cfg.network);

    boost::asio::io_context io;
    policy::FeePolicy feePolicy(1, 100000, 100);
    mempool::Mempool pool(feePolicy);
    pool.SetValidationContext(params, /*height=*/0, {});

    wallet::KeyStore store;
    wallet::WalletBackend wallet(store);
    try {
        wallet.SetHDSeed(SeedFromPath(cfg.datadir));
    } catch (...) {
        // best effort; wallet will still function for watching balances
    }

    txindex::TxIndex index;
    index.Open(cfg.datadir + "/txindex");

    net::P2PNode p2p(io, cfg.p2pport);
    p2p.SetLocalHeight(static_cast<uint32_t>(index.BlockCount()));

    sidechain::wasm::ExecutionEngine wasmEngine;
    sidechain::state::StateStore sidechainState;
    sidechain::rpc::WasmRpcService wasmService(wasmEngine, sidechainState);

    rpc::RPCServer rpc(io, cfg.rpcuser, cfg.rpcpassword, cfg.rpcport);
    rpc.SetBlockStorePath(cfg.datadir + "/blocks.dat");
    rpc.AttachCoreHandlers(pool, wallet, index, p2p);
    rpc.AttachSidechainHandlers(wasmService);

    if (cfg.listen) {
        p2p.Start();
    }
    rpc.Start();

    std::cout << "drachmad started (" << cfg.network << ")\n";
    std::cout << "RPC listening on port " << cfg.rpcport << " user=" << cfg.rpcuser << "\n";
    std::cout << "P2P listening on port " << cfg.p2pport << (cfg.listen ? "" : " (disabled)") << "\n";

    std::signal(SIGINT, [](int) { std::exit(0); });
    std::signal(SIGTERM, [](int) { std::exit(0); });
    io.run();
    return 0;
}
