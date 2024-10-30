#ifndef UTILITY_STRING_UTILS_H
#define UTILITY_STRING_UTILS_H

#include <string>

#if defined(_WIN32)
    std::wstring utf8StringToWideString(const std::string& string);
    std::string wideStringToUtf8String(const std::wstring& string);

    using FatfsCharacter = wchar_t;

    inline std::basic_string<wchar_t> utf8StringToFatfsString(const std::string &string) {
        return utf8StringToWideString(string);
    }

    inline std::string fatfsStringToUtf8String(const std::basic_string<wchar_t> &string) {
        return wideStringToUtf8String(string);
    }

#define FF_T(x) L##x
#else
    inline const std::basic_string<char> &utf8StringToFatfsString(const std::string &string) {
        return string;
    }

    inline const std::string &fatfsStringToUtf8String(const std::basic_string<char> &string) {
        return string;
    }
#define FF_T(x) x
#endif

    using FatfsString = std::basic_string<FatfsCharacter>;

#endif
