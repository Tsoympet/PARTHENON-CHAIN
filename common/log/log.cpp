#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace log {

enum class Level { DEBUG, INFO, WARN, ERROR };

namespace {
std::mutex g_lock;
std::ofstream g_file;
Level g_level = Level::INFO;

std::string Now()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

const char* ToString(Level l)
{
    switch (l) {
        case Level::DEBUG: return "DEBUG";
        case Level::INFO:  return "INFO";
        case Level::WARN:  return "WARN";
        case Level::ERROR: return "ERROR";
    }
    return "INFO";
}

} // namespace

void SetLevel(Level level)
{
    std::scoped_lock lock(g_lock);
    g_level = level;
}

void SetFile(const std::filesystem::path& path)
{
    std::scoped_lock lock(g_lock);
    if (!path.empty()) {
        std::filesystem::create_directories(path.parent_path());
        g_file.open(path, std::ios::app);
    }
}

void Write(Level level, const std::string& msg)
{
    std::scoped_lock lock(g_lock);
    if (level < g_level)
        return;
    auto pid =
#ifdef _WIN32
        _getpid();
#else
        getpid();
#endif
    std::string line = Now() + " [" + ToString(level) + "] (" + std::to_string(pid) + ":" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ") " + msg + "\n";
    std::cerr << line;
    if (g_file.is_open())
        g_file << line;
}

void Debug(const std::string& msg) { Write(Level::DEBUG, msg); }
void Info(const std::string& msg)  { Write(Level::INFO, msg); }
void Warn(const std::string& msg)  { Write(Level::WARN, msg); }
void Error(const std::string& msg) { Write(Level::ERROR, msg); }

} // namespace log
