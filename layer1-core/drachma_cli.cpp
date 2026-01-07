#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

#include "../common/version.h"

namespace http = boost::beast::http;

namespace {

void PrintVersion()
{
    std::cout << PARTHENON_CHAIN_NAME << " (" << PARTHENON_CHAIN_CODENAME << ") CLI version "
              << DRACHMA_VERSION_STRING << "\n";
    std::cout << "Build: " << DRACHMA_BUILD_TYPE << "\n";
}

void PrintHelp()
{
    std::cout << "Usage: drachma-cli [options] <method> [params_json]\n\n";
    std::cout << PARTHENON_CHAIN_NAME << " - RPC command-line interface\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help                Show this help message and exit\n";
    std::cout << "  --version             Show version information and exit\n";
    std::cout << "  -rpcuser=<user>       RPC username (default: user)\n";
    std::cout << "  -rpcpassword=<pass>   RPC password (default: pass)\n";
    std::cout << "  -rpcport=<port>       RPC port (default: 8332)\n";
    std::cout << "  -rpcconnect=<host>    RPC host (default: 127.0.0.1)\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  method                RPC method to call\n";
    std::cout << "  params_json           JSON parameters (default: null)\n\n";
    std::cout << "Examples:\n";
    std::cout << "  drachma-cli getblockcount\n";
    std::cout << "  drachma-cli getblock '[\"blockhash\"]'\n";
    std::cout << "  drachma-cli -rpcuser=myuser -rpcpassword=mypass getinfo\n\n";
    std::cout << "For more information, visit: https://github.com/Tsoympet/PARTHENON-CHAIN\n";
}

} // namespace

int main(int argc, char* argv[])
{
    std::string host{"127.0.0.1"};
    std::string user{"user"};
    std::string pass{"pass"};
    uint16_t port{8332};
    std::string method;
    std::string params{"null"};

    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--help" || arg == "-h") {
            PrintHelp();
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            PrintVersion();
            return 0;
        } else if (arg.rfind("-rpcuser=", 0) == 0) user = arg.substr(9);
        else if (arg.rfind("-rpcpassword=", 0) == 0) pass = arg.substr(13);
        else if (arg.rfind("-rpcport=", 0) == 0) {
            try {
                port = static_cast<uint16_t>(std::stoul(arg.substr(9)));
            } catch (...) {
                std::cerr << "Invalid rpcport value\n";
                return 1;
            }
        }
        else if (arg.rfind("-rpcconnect=", 0) == 0) host = arg.substr(12);
        else if (method.empty()) method = arg;
        else if (params == "null") params = arg;
    }

    if (method.empty()) {
        PrintHelp();
        return 1;
    }

    std::string auth = user + ":" + pass;
    std::string encoded;
    encoded.resize(boost::beast::detail::base64::encoded_size(auth.size()));
    auto len = boost::beast::detail::base64::encode(&encoded[0], auth.data(), auth.size());
    encoded.resize(len);

    try {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::resolver resolver(ioc);
        boost::beast::tcp_stream stream(ioc);
        auto const results = resolver.resolve(host, std::to_string(port));
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, "/", 11};
        req.set(http::field::host, host);
        req.set(http::field::authorization, "Basic " + encoded);
        req.set(http::field::content_type, "application/json");
        req.body() = std::string("{\"method\":\"") + method + "\",\"params\":" + params + "}";
        req.prepare_payload();

        http::write(stream, req);
        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);
        std::cout << res.body() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC call failed: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
