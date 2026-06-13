#pragma once

#include <string>
#include <string_view>
#include <windows.h>

static inline bool IsValidUtf8(std::string_view string)
{
    if (string.empty())
        return true;

    return ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        string.data(),
        (int)string.size(),
        nullptr,
        0
    ) > 0;
}

static inline bool ConvertMultiByteToWide(UINT cp, DWORD flags, std::string_view in, std::wstring& out)
{
    out.clear();
    if (in.empty())
        return true;

    int wlen = ::MultiByteToWideChar(cp, flags, in.data(), (int)in.size(), nullptr, 0);
    if (wlen <= 0)
        return false;

    out.resize((size_t)wlen);
    wlen = ::MultiByteToWideChar(cp, flags, in.data(), (int)in.size(), out.data(), wlen);
    return wlen > 0;
}

static inline bool ConvertWideToMultiByte(UINT cp, DWORD flags, const std::wstring& in, std::string& out)
{
    out.clear();
    if (in.empty())
        return true;

    int len = ::WideCharToMultiByte(cp, flags, in.data(), (int)in.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return false;

    out.resize((size_t)len);
    len = ::WideCharToMultiByte(cp, flags, in.data(), (int)in.size(), out.data(), len, nullptr, nullptr);
    return len > 0;
}

static inline std::string EnsureUtf8ForCef(std::string_view string)
{
    if (string.empty())
        return {};

    if (IsValidUtf8(string))
        return std::string(string);

    std::wstring wstring;
    if (!ConvertMultiByteToWide(::GetACP(), 0, string, wstring))
        return std::string(string);

    std::string out;
    if (!ConvertWideToMultiByte(CP_UTF8, 0, wstring, out))
        return std::string(string);

    return out;
}