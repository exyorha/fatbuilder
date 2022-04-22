#ifndef FILESYSTEM_RAW_BLOCK_DEVICE_H
#define FILESYSTEM_RAW_BLOCK_DEVICE_H

#include "IBlockDevice.h"

#if defined(_WIN32)
#include <Windows.h>

#include <memory>
#include <type_traits>
#else
#include <unistd.h>
#endif

#include <filesystem>
#include <cstdint>

class RawBlockDevice final : public IBlockDevice {
public:
	RawBlockDevice(std::filesystem::path&& path, uint64_t size);
	~RawBlockDevice() override;

	void read(uint64_t offset, void* buffer, size_t size) override;
	void write(uint64_t offset, const void* buffer, size_t size) override;
	void flush() override;

	uint64_t mediaSize() const override;
	virtual unsigned int allocationUnit() const override;

private:
	void doRead(uint64_t offset, void* buffer, size_t size);
	void doWrite(uint64_t offset, const void* buffer, size_t size);

#if defined(_WIN32)
	struct WindowsHandleDeleter {
		inline void operator()(HANDLE handle) const {
			CloseHandle(handle);
		}
	};
#else
	struct ManagedHandle {
		ManagedHandle() : fd(-1) {

		}

		~ManagedHandle() {
			if(fd >= 0)
				close(fd);
		}

		ManagedHandle(const ManagedHandle &other) = delete;
		ManagedHandle &operator =(const ManagedHandle &other) = delete;

		int fd;
	};
#endif

#if defined(_WIN32)
	std::unique_ptr<std::remove_pointer<HANDLE>::type, WindowsHandleDeleter> m_handle;
	alignas(4) unsigned char m_bounceBuffer[8192];
#else
	ManagedHandle m_handle;
#endif
	uint64_t m_mediaSize;
	unsigned int m_allocationUnit;
#if defined(_WIN32)
#endif
};

#endif
