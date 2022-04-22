#ifndef UTILITY_STRING_UTILS_H
#define UTILITY_STRING_UTILS_H

#include <string>

#if defined(_WIN32)
    std::wstring utf8StringToWideString(const std::string& string);
    std::string wideStringToUtf8String(const std::wstring& string);

    using FatfsString = std::wstring;

    inline FatfsString utf8StringToFatfsString(const std::string &string) {
        return utf8StringToWideString(string);
    }

    inline std::string fatfsStringToUtf8String(const FatfsString &string) {
        return wideStringToUtf8String(string);
    }
#else
    using FatfsString = std::string;

    inline const FatfsString &utf8StringToFatfsString(const std::string &string) {
        return string;
    }

    inline const std::string &fatfsStringToUtf8String(const FatfsString &string) {
        return string;
    }
#endif

#endif
