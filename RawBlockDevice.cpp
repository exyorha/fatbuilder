#include "RawBlockDevice.h"

#include <Windows.h>
#include <comdef.h>

RawBlockDevice::RawBlockDevice(std::filesystem::path&& path, uint64_t size) : m_mediaSize(size), m_allocationUnit(512) {
	auto rawHandle = CreateFile(
		path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if(rawHandle == INVALID_HANDLE_VALUE)
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

	m_handle.reset(rawHandle);

	LARGE_INTEGER pos;
	pos.QuadPart = m_mediaSize;
	if(!SetFilePointerEx(m_handle.get(), pos, nullptr, FILE_BEGIN))
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));

	if(!SetEndOfFile(m_handle.get()))
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));
}

RawBlockDevice::~RawBlockDevice() = default;

void RawBlockDevice::read(uint64_t offset, void* buffer, size_t size) {
	if (reinterpret_cast<uint32_t>(buffer) & 3) {
		auto cbuffer = static_cast<uint8_t*>(buffer);

		while (size > 0) {
			auto chunk = std::min<size_t>(size, sizeof(m_bounceBuffer));

			doRead(offset, m_bounceBuffer, size);

			memcpy(cbuffer, m_bounceBuffer, chunk);

			offset += chunk;
			cbuffer += chunk;
			size -= chunk;			
		}
	}
	else {
		doRead(offset, buffer, size);
	}
}

void RawBlockDevice::doRead(uint64_t offset, void *buffer, size_t size) {
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = static_cast<DWORD>(offset);
	overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

	DWORD bytesRead;

	if (!ReadFile(m_handle.get(), buffer, size, &bytesRead, &overlapped)) {
		auto error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			if (!GetOverlappedResult(m_handle.get(), &overlapped, &bytesRead, TRUE))
				error = GetLastError();
			else
				return;
		}

		_com_raise_error(HRESULT_FROM_WIN32(error));
	}
}

void RawBlockDevice::write(uint64_t offset, const void* buffer, size_t size) {
	if (reinterpret_cast<uint32_t>(buffer) & 3) {
		auto cbuffer = static_cast<const uint8_t*>(buffer);

		while (size > 0) {
			auto chunk = std::min<size_t>(size, sizeof(m_bounceBuffer));

			memcpy(m_bounceBuffer, cbuffer, chunk);

			doWrite(offset, m_bounceBuffer, size);

			offset += chunk;
			cbuffer += chunk;
			size -= chunk;
		}
	}
	else {
		doWrite(offset, buffer, size);
	}
}

void RawBlockDevice::doWrite(uint64_t offset, const void* buffer, size_t size) {
	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));
	overlapped.Offset = static_cast<DWORD>(offset);
	overlapped.OffsetHigh = static_cast<DWORD>(offset >> 32);

	DWORD bytesRead;

	if (!WriteFile(m_handle.get(), buffer, size, &bytesRead, &overlapped)) {
		auto error = GetLastError();
		if (error == ERROR_IO_PENDING) {
			if (!GetOverlappedResult(m_handle.get(), &overlapped, &bytesRead, TRUE))
				error = GetLastError();
			else
				return;
		}

		_com_raise_error(HRESULT_FROM_WIN32(error));
	}
}

void RawBlockDevice::flush() {
	if (!FlushFileBuffers(m_handle.get()))
		_com_raise_error(HRESULT_FROM_WIN32(GetLastError()));
}

uint64_t RawBlockDevice::mediaSize() const {
	return m_mediaSize;
}

unsigned int RawBlockDevice::allocationUnit() const {
	return m_allocationUnit;
}