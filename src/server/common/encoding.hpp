#pragma once

#ifdef _WIN32
#include <windows.h>

inline std::string Utf8ToAnsi(const std::string& utf8)
{
    if (utf8.empty()) 
        return {};

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
    if (wlen == 0) 
        return utf8;

    std::wstring wstr(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), &wstr[0], wlen);

    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wlen, nullptr, 0, nullptr, nullptr);
    if (alen == 0) 
        return utf8;

    std::string ansistr(alen, '\0');
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wlen, &ansistr[0], alen, nullptr, nullptr);

    return ansistr;
}
#else
inline std::string Utf8ToAnsi(const std::string& utf8)
{
    return utf8;
}
#endif