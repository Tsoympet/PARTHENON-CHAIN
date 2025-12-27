#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <sstream>

namespace rpc {

class RPCServer {
public:
    using Handler = std::function<std::string(const std::string&)>;

    RPCServer(std::string user, std::string pass)
        : m_user(std::move(user)), m_pass(std::move(pass)) {}

    void Register(const std::string& method, Handler handler)
    {
        m_handlers[method] = std::move(handler);
    }

    std::string HandleRequest(const std::string& user, const std::string& pass, const std::string& method, const std::string& params)
    {
        if (user != m_user || pass != m_pass)
            throw std::runtime_error("unauthorized");
        auto it = m_handlers.find(method);
        if (it == m_handlers.end())
            throw std::runtime_error("method not found");
        return it->second(params);
    }

private:
    std::string m_user;
    std::string m_pass;
    std::unordered_map<std::string, Handler> m_handlers;
};

} // namespace rpc
