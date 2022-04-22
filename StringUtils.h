#ifndef UTILITY_STRING_UTILS_H
#define UTILITY_STRING_UTILS_H

#include <string>

std::wstring utf8StringToWideString(const std::string& string);
std::string wideStringToUtf8String(const std::wstring& string);

#endif
