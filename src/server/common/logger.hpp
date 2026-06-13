#pragma once

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <string>

#include "bridge.hpp"

enum class CefLogLevel : uint8_t
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

struct LogEvent
{
    CefLogLevel level;
    std::string message;
    std::chrono::system_clock::time_point ts;
    const char* file;
    int line;
    const char* func;
};

class Logger
{
public:
    explicit Logger(CefLogLevel minLevel = CefLogLevel::Info) : minLevel_(minLevel) {}
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void SetBridge(IPlatformBridge* bridge)
    {
        bridge_ = bridge;
    }

    void SetLevel(CefLogLevel lvl)
    {
        minLevel_.store(lvl, std::memory_order_relaxed);
    }

    CefLogLevel GetLevel() const
    {
        return minLevel_.load(std::memory_order_relaxed);
    }

    void SetDecorate(bool enabled)
    {
        decorate_.store(enabled, std::memory_order_relaxed);
    }

    void Logf(CefLogLevel lvl, const char* file, int line, const char* func, const char* fmt, ...);
    void Logv(CefLogLevel lvl, const char* file, int line, const char* func, const char* fmt, va_list ap);

    void Logs(CefLogLevel lvl, const char* file, int line, const char* func, std::string msg);

private:
    static std::string VFormat(const char* fmt, va_list ap);
    std::string FormatLine(const LogEvent& ev) const;
    void WriteToBridge(const LogEvent& ev);

private:
    IPlatformBridge* bridge_ = nullptr;

    std::atomic<CefLogLevel> minLevel_;
    std::atomic<bool> decorate_{false};
};


namespace logging
{
    void SetLogger(Logger* logger);
    Logger* GetLogger() noexcept;
}

#define LOG_INFO(fmt, ...)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto* _lg = ::logging::GetLogger())                                                                        \
        {                                                                                                              \
            _lg->Logf(CefLogLevel::Info, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                               \
        }                                                                                                              \
    } while (0)
#define LOG_WARN(fmt, ...)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto* _lg = ::logging::GetLogger())                                                                        \
        {                                                                                                              \
            _lg->Logf(CefLogLevel::Warn, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                               \
        }                                                                                                              \
    } while (0)
#define LOG_ERROR(fmt, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto* _lg = ::logging::GetLogger())                                                                        \
        {                                                                                                              \
            _lg->Logf(CefLogLevel::Error, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                              \
        }                                                                                                              \
    } while (0)
#ifndef NDEBUG
#define LOG_DEBUG(fmt, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (auto* _lg = ::logging::GetLogger())                                                                        \
        {                                                                                                              \
            _lg->Logf(CefLogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__);                              \
        }                                                                                                              \
    } while (0)
#else
#define LOG_DEBUG(fmt, ...) ((void)0)
#endif


#define LOGX_INFO(lg, fmt, ...) (lg).Logf(CefLogLevel::Info, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGX_WARN(lg, fmt, ...) (lg).Logf(CefLogLevel::Warn, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOGX_ERROR(lg, fmt, ...) (lg).Logf(CefLogLevel::Error, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#ifndef NDEBUG
#define LOGX_DEBUG(lg, fmt, ...) (lg).Logf(CefLogLevel::Debug, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
#define LOGX_DEBUG(lg, fmt, ...) ((void)0)
#endif