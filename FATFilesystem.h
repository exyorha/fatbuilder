#ifndef FILESYSTEM_FAT_FILESYSTEM_H
#define FILESYSTEM_FAT_FILESYSTEM_H

#include "IFilesystem.h"
#include "IFile.h"
#include "StringUtils.h"

#include <memory>
#include <array>
#include <filesystem>

#include <ff.h>
#include <diskio.h>

class IBlockDevice;

class FATFilesystem final : public IFilesystem {
public:
	explicit FATFilesystem(std::unique_ptr<IBlockDevice>&& storage);
	~FATFilesystem() override;

	bool createDirectory(const FatfsString& name) override;
	std::unique_ptr<IFile> open(const FatfsString& name, const FatfsString& mode) override;
	virtual void setAttributes(const FatfsString& name, unsigned int attributes, unsigned int attributeMask) override;

private:
	class AllocatedDriveNumber {
	public:
		explicit AllocatedDriveNumber(FATFilesystem *owner);
		~AllocatedDriveNumber();

		AllocatedDriveNumber(const AllocatedDriveNumber& other) = delete;
		AllocatedDriveNumber &operator =(const AllocatedDriveNumber& other) = delete;

		inline operator unsigned int() const {
			return m_driveNumber;
		}

		static inline FATFilesystem* getFS(unsigned int number) { return m_allocatedDrives[number]; }

	private:
		static std::array<FATFilesystem *, 10> m_allocatedDrives;
		unsigned int m_driveNumber;
	};

	class FATFile final : public IFile {
	public:
		FATFile(FATFilesystem *parent, const FatfsString& name, const FatfsString& mode);
		~FATFile() override;

		int64_t seek(int64_t offset, SeekWhence whence) override;
		size_t read(void* data, size_t size) override;
		size_t write(const void* data, size_t size) override;

	private:
		FATFilesystem* m_parent;
		FIL m_file;
	};

	void translateError(FRESULT result);
	void installBootCode();

	friend DSTATUS disk_initialize(BYTE pdrv);
	friend DSTATUS disk_status(BYTE pdrv);
	friend DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
	friend DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
	friend DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

	FatfsString pathToPartition(const FatfsString &path = FatfsString());

	AllocatedDriveNumber m_driveNumber;
	std::unique_ptr<IBlockDevice> m_storage;
	unsigned char m_workArea[128 * FF_MAX_SS];
	FATFS m_fs;

	static const unsigned char m_mbrCode[512];
	static const unsigned char m_pbrCode_12_16[512];
	static const unsigned char m_pbrCode_32[1536];
};

#endif
