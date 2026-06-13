#include "logger.hpp"

#include <atomic>
#include <cstdio>
#include <sstream>

namespace
{
    std::atomic<Logger*> g_logger{nullptr};
}

void logging::SetLogger(Logger* logger)
{
    g_logger.store(logger, std::memory_order_relaxed);
}

Logger* logging::GetLogger() noexcept
{
    return g_logger.load(std::memory_order_relaxed);
}

void Logger::Logf(CefLogLevel lvl, const char* file, int line, const char* func, const char* fmt, ...)
{
    if (lvl < GetLevel())
        return;
    va_list ap;
    va_start(ap, fmt);
    auto s = VFormat(fmt, ap);
    va_end(ap);
    Logs(lvl, file, line, func, std::move(s));
}

void Logger::Logv(CefLogLevel lvl, const char* file, int line, const char* func, const char* fmt, va_list ap)
{
    if (lvl < GetLevel())
        return;
    Logs(lvl, file, line, func, VFormat(fmt, ap));
}

void Logger::Logs(CefLogLevel lvl, const char* file, int line, const char* func, std::string msg)
{
    if (lvl < GetLevel())
        return;

    LogEvent ev{};
    ev.level = lvl;
    ev.message = std::move(msg);
    ev.ts = std::chrono::system_clock::now();
    ev.file = file;
    ev.line = line;
    ev.func = func;
    WriteToBridge(ev);
}

std::string Logger::VFormat(const char* fmt, va_list ap)
{
    char buf[1024];
    va_list ap2;
    va_copy(ap2, ap);

#if defined(_MSC_VER)
    int n = _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
#else
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
#endif

    if (n >= 0 && static_cast<size_t>(n) < sizeof(buf))
    {
        va_end(ap2);
        return std::string(buf, static_cast<size_t>(n));
    }

    size_t size = n > 0 ? static_cast<size_t>(n) + 1 : 2048;

    std::string out;
    out.resize(size);

    vsnprintf(&out[0], size, fmt, ap2);
    va_end(ap2);

    if (!out.empty() && out.back() == '\0')
        out.pop_back();

    return out;
}

std::string Logger::FormatLine(const LogEvent& ev) const
{
    if (!decorate_.load(std::memory_order_relaxed))
    {
        return ev.message;
    }

    using namespace std::chrono;
    auto tt = system_clock::to_time_t(ev.ts);
    std::tm tm{};

#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif

    char tsbuf[32];
    std::snprintf(tsbuf,
                  sizeof(tsbuf),
                  "%04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900,
                  tm.tm_mon + 1,
                  tm.tm_mday,
                  tm.tm_hour,
                  tm.tm_min,
                  tm.tm_sec);

    std::ostringstream oss;
    oss << tsbuf << ' ';
    if (ev.file)
        oss << ev.file << ':' << ev.line << ' ';
    if (ev.func)
        oss << ev.func << "() ";
    oss << "- " << ev.message;
    return oss.str();
}

void Logger::WriteToBridge(const LogEvent& ev)
{
    if (!bridge_)
        return;

    const std::string line = FormatLine(ev);
    switch (ev.level)
    {
        case CefLogLevel::Error:
        case CefLogLevel::Critical:
            bridge_->LogError(line);
            break;
        case CefLogLevel::Warn:
            bridge_->LogWarn(line);
            break;
        case CefLogLevel::Info:
            bridge_->LogInfo(line);
            break;
        default:
            bridge_->LogDebug(line);
            break;
    }
}