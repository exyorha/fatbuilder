#include "StringUtils.h"
#if defined(_WIN32)
#include <Windows.h>
#include <comdef.h>

std::wstring utf8StringToWideString(const std::string& string) {
    if (string.empty())
        return {};

    int length = MultiByteToWideChar(CP_UTF8, 0, string.data(), string.size(), nullptr, 0);
    if (length == 0)
        _com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

    std::wstring receiver;
    receiver.resize(length);

    length = MultiByteToWideChar(CP_UTF8, 0, string.data(), string.size(), receiver.data(), receiver.size());
    if (length == 0)
        _com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

    receiver.resize(length);

    return receiver;
}

std::string wideStringToUtf8String(const std::wstring& string) {
    if (string.empty())
        return {};

    int length = WideCharToMultiByte(CP_UTF8, 0, string.data(), string.size(), nullptr, 0, nullptr, nullptr);
    if (length == 0)
        _com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

    std::string receiver;
    receiver.resize(length);

    length = WideCharToMultiByte(CP_UTF8, 0, string.data(), string.size(), receiver.data(), receiver.size(), nullptr, nullptr);
    if (length == 0)
        _com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

    receiver.resize(length);

    return receiver;
}
#endif
