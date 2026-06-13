#pragma once

#include <fmt/format.h>
#include <fmt/chrono.h>
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <chrono>

class Logger
{
public:
    Logger() = default;
    ~Logger() { if (log_stream_.is_open()) log_stream_.close(); }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void SetDebugMode(bool enabled) { debug_enabled_ = enabled; }
    bool IsDebugMode() const { return debug_enabled_; }

    void SetLogFile(const std::string& path)
    {
        std::scoped_lock lock(log_mutex_);
        if (log_stream_.is_open()) log_stream_.close();
        log_stream_.open(path, std::ios::app);
        log_stream_ << std::unitbuf;
        if (!log_stream_.is_open()) {
            std::cerr << "[Logger] Failed to open log file: " << path << std::endl;
        }
    }

    void Log(const std::string& final_message)
    {
        std::string line = fmt::format("[{:%Y-%m-%d %H:%M:%S}] {}\n", std::chrono::system_clock::now(), final_message);
        std::scoped_lock lock(log_mutex_);
        std::cout << line;
        if (log_stream_.is_open()) log_stream_ << line;
    }

    template <typename... Args> void Info(fmt::format_string<Args...> f, Args&&... a)  { Log(fmt::format("[INFO] {}",  fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void Warn(fmt::format_string<Args...> f, Args&&... a)  { Log(fmt::format("[WARN] {}",  fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void Error(fmt::format_string<Args...> f, Args&&... a) { Log(fmt::format("[ERROR] {}", fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void Fatal(fmt::format_string<Args...> f, Args&&... a) { Log(fmt::format("[FATAL] {}", fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void Debug(fmt::format_string<Args...> f, Args&&... a) { if (debug_enabled_) Log(fmt::format("[DEBUG] {}", fmt::format(f, std::forward<Args>(a)...))); }

    template <typename... Args> void InfoEx(const char* file, int line, fmt::format_string<Args...> f, Args&&... a)  { Log(fmt::format("[INFO] [{}:{}] {}",  file, line, fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void WarnEx(const char* file, int line, fmt::format_string<Args...> f, Args&&... a)  { Log(fmt::format("[WARN] [{}:{}] {}",  file, line, fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void ErrorEx(const char* file, int line, fmt::format_string<Args...> f, Args&&... a) { Log(fmt::format("[ERROR] [{}:{}] {}", file, line, fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void FatalEx(const char* file, int line, fmt::format_string<Args...> f, Args&&... a) { Log(fmt::format("[FATAL] [{}:{}] {}", file, line, fmt::format(f, std::forward<Args>(a)...))); }
    template <typename... Args> void DebugEx(const char* file, int line, fmt::format_string<Args...> f, Args&&... a) { if (debug_enabled_) Log(fmt::format("[DEBUG] [{}:{}] {}", file, line, fmt::format(f, std::forward<Args>(a)...))); }

private:
    std::ofstream log_stream_;
    mutable std::mutex log_mutex_;
    bool debug_enabled_ = false;
};

namespace logging {
    inline Logger* g_logger = nullptr;
    inline void SetLogger(Logger* logger) { g_logger = logger; }
    inline Logger* GetLogger() { return g_logger; }
}

constexpr const char* Basename(const char* path)
{
    const char* file = path;
    for (const char* p = path; *p; ++p) if (*p == '/' || *p == '\\') file = p + 1;
    return file;
}

#define LOG_INFO(...)  do { if (auto* _L = logging::GetLogger()) { _L->Info(__VA_ARGS__); } } while(0)
#define LOG_WARN(...)  do { if (auto* _L = logging::GetLogger()) { _L->Warn(__VA_ARGS__); } } while(0)
#define LOG_ERROR(...) do { if (auto* _L = logging::GetLogger()) { _L->Error(__VA_ARGS__); } } while(0)
#define LOG_FATAL(...) do { if (auto* _L = logging::GetLogger()) { _L->Fatal(__VA_ARGS__); } } while(0)
#define LOG_DEBUG(...) do { if (auto* _L = logging::GetLogger()) { _L->Debug(__VA_ARGS__); } } while(0)

#define LOG_INFO_EX(...)  do { if (auto* _L = logging::GetLogger()) { _L->InfoEx (Basename(__FILE__), __LINE__, __VA_ARGS__); } } while(0)
#define LOG_WARN_EX(...)  do { if (auto* _L = logging::GetLogger()) { _L->WarnEx (Basename(__FILE__), __LINE__, __VA_ARGS__); } } while(0)
#define LOG_ERROR_EX(...) do { if (auto* _L = logging::GetLogger()) { _L->ErrorEx(Basename(__FILE__), __LINE__, __VA_ARGS__); } } while(0)
#define LOG_FATAL_EX(...) do { if (auto* _L = logging::GetLogger()) { _L->FatalEx(Basename(__FILE__), __LINE__, __VA_ARGS__); } } while(0)
#define LOG_DEBUG_EX(...) do { if (auto* _L = logging::GetLogger()) { _L->DebugEx(Basename(__FILE__), __LINE__, __VA_ARGS__); } } while(0)