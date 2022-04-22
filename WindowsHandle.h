#ifndef UTILITY_WINDOWS_HANDLE_H
#define UTILITY_WINDOWS_HANDLE_H

#include <Windows.h>

#include <memory>
#include <type_traits>

struct WindowsHandleDeleter {
	inline void operator()(HANDLE handle) const {
		CloseHandle(handle);
	}
};

using WindowsHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, WindowsHandleDeleter>;

#endif
