#ifndef FILESYSTEM_IBLOCKDEVICE_H
#define FILESYSTEM_IBLOCKDEVICE_H

#include <stdint.h>

class IBlockDevice {
protected:
	IBlockDevice();

public:
	virtual ~IBlockDevice();

	IBlockDevice(IBlockDevice& other) = delete;
	IBlockDevice &operator =(IBlockDevice& other) = delete;

	virtual void read(uint64_t offset, void* buffer, size_t size) = 0;
	virtual void write(uint64_t offset, const void* buffer, size_t size) = 0;
	virtual void flush() = 0;

	virtual uint64_t mediaSize() const = 0;
	virtual unsigned int allocationUnit() const = 0;
};

#endif
