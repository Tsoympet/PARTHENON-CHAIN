#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace http = boost::beast::http;

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
        if (arg.rfind("-rpcuser=", 0) == 0) user = arg.substr(9);
        else if (arg.rfind("-rpcpassword=", 0) == 0) pass = arg.substr(13);
        else if (arg.rfind("-rpcport=", 0) == 0) {
            try {
                port = static_cast<uint16_t>(std::stoul(arg.substr(9)));
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid rpcport value: not a number\n";
                return 1;
            } catch (const std::out_of_range& e) {
                std::cerr << "Invalid rpcport value: out of range\n";
                return 1;
            }
        }
        else if (arg.rfind("-rpcconnect=", 0) == 0) host = arg.substr(12);
        else if (method.empty()) method = arg;
        else if (params == "null") params = arg;
    }

    if (method.empty()) {
        std::cerr << "Usage: drachma-cli [-rpcuser=U] [-rpcpassword=P] [-rpcport=N] [-rpcconnect=HOST] method [params_json]\n";
        std::cerr << "params_json should be valid JSON (e.g., null, \"\\\"DRM\\\"\", or an array like [\"DRM\"])\n";
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
