#include "RawBlockDevice.h"

#if defined(_WIN32)
#include <Windows.h>
#include <comdef.h>
#else
#include <fcntl.h>
#include <system_error>
#endif

RawBlockDevice::RawBlockDevice(std::filesystem::path&& path, uint64_t size) : m_mediaSize(size), m_allocationUnit(512) {
#if defined(_WIN32)
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
#else
	if constexpr (sizeof(m_mediaSize) != sizeof(off_t)) {
		if(m_mediaSize > std::numeric_limits<off_t>::max())
			throw std::runtime_error("media size is out of range");
	}

	m_handle.fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
	if(m_handle.fd < 0)
		throw std::system_error(errno, std::generic_category());

	auto result = ftruncate(m_handle.fd, m_mediaSize);
	if(result < 0)
		throw std::system_error(errno, std::generic_category());
#endif
}

RawBlockDevice::~RawBlockDevice() = default;

#if defined(_WIN32)
void RawBlockDevice::read(uint64_t offset, void* buffer, size_t size) {
	if (reinterpret_cast<uintptr_t>(buffer) & 3) {
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
	if (reinterpret_cast<uintptr_t>(buffer) & 3) {
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
#else

void RawBlockDevice::read(uint64_t offset, void* buffer, size_t size) {
	if constexpr (sizeof(offset) != sizeof(off_t)) {
		if(offset > std::numeric_limits<off_t>::max())
			throw std::runtime_error("offset is out of range");
	}

	if(offset + size > m_mediaSize || offset + size < offset)
		throw std::runtime_error("the access requested is out of the media bounds");

	auto result = pread(m_handle.fd, buffer, size, offset);
	if(result < 0)
		throw std::system_error(errno, std::generic_category());

	if(static_cast<size_t>(result) != size)
		throw std::runtime_error("short read");
}

void RawBlockDevice::write(uint64_t offset, const void* buffer, size_t size) {
	if constexpr (sizeof(offset) != sizeof(off_t)) {
		if(offset > std::numeric_limits<off_t>::max())
			throw std::runtime_error("offset is out of range");
	}

	if(offset + size > m_mediaSize || offset + size < offset)
		throw std::runtime_error("the access requested is out of the media bounds");

	auto result = pwrite(m_handle.fd, buffer, size, offset);
	if(result < 0)
		throw std::system_error(errno, std::generic_category());

	if(static_cast<size_t>(result) != size)
		throw std::runtime_error("short write");
}

void RawBlockDevice::flush() {
	auto result = fsync(m_handle.fd);
	if(result < 0)
		throw std::system_error(errno, std::generic_category());
}

#endif

uint64_t RawBlockDevice::mediaSize() const {
	return m_mediaSize;
}

unsigned int RawBlockDevice::allocationUnit() const {
	return m_allocationUnit;
}
