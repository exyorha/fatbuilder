#include "FATFilesystem.h"
#include "IBlockDevice.h"
#include "WindowsResources.h"

#include <time.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <stdexcept>

std::array<FATFilesystem *, 10> FATFilesystem::AllocatedDriveNumber::m_allocatedDrives;

FATFilesystem::AllocatedDriveNumber::AllocatedDriveNumber(FATFilesystem *owner) {
	for (size_t index = 0; index < m_allocatedDrives.size(); index++) {
		if (!m_allocatedDrives[index]) {
			m_allocatedDrives[index] = owner;
			m_driveNumber = index;
			return;
		}
	}

	throw std::runtime_error("too many FAT drives");
}

FATFilesystem::AllocatedDriveNumber::~AllocatedDriveNumber() {
	m_allocatedDrives[m_driveNumber] = nullptr;
}

FATFilesystem::FATFilesystem(std::unique_ptr<IBlockDevice>&& storage) : m_driveNumber(this), m_storage(std::move(storage)) {
	static const LBA_t plist[2] = { 100, 0 };

	translateError(f_mkfs(pathToPartition().c_str(), nullptr, m_workArea, sizeof(m_workArea)));

	installBootCode();

	translateError(f_mount(&m_fs, pathToPartition().c_str(), 1));
}


FATFilesystem::~FATFilesystem() {
	f_mount(nullptr, pathToPartition().c_str(), 0);
}

void FATFilesystem::translateError(FRESULT result) {
	if (result != FR_OK)
		throw std::runtime_error("fatfs call failed with status " + std::to_string(result));
}

void FATFilesystem::installBootCode() {
	if (disk_read(m_driveNumber, m_workArea, 0, 1) != RES_OK)
		throw std::runtime_error("failed to read MBR");

	const void* mbrCode;
	size_t mbrCodeSize;
	getRCDATA(1, &mbrCode, &mbrCodeSize);

	memcpy(m_workArea, mbrCode, std::min<size_t>(446, mbrCodeSize));

	m_workArea[446] = 0x80; // Mark partition as active
	m_workArea[450] = 0x0E; // Set partition type as FAT16-LBA

	auto firstBlock = *reinterpret_cast<const uint32_t*>(&m_workArea[446 + 8]);

	if (disk_write(m_driveNumber, m_workArea, 0, 1) != RES_OK)
		throw std::runtime_error("failed to rewrite MBR");

	if (disk_read(m_driveNumber, m_workArea, firstBlock, 1) != RES_OK)
		throw std::runtime_error("failed to read PBR");

	auto signature = reinterpret_cast<const char*>(&m_workArea[0x36]);

	if (strncmp(signature, "FAT32   ", 8) == 0) {
		throw std::runtime_error("FAT32 boot is not implemented yet");
	}
	else {
		const void* pbrCode;
		size_t pbrCodeSize;
		getRCDATA(2, &pbrCode, &pbrCodeSize);

		if (pbrCodeSize != 512)
			throw std::runtime_error("PBR should be exactly 512 bytes in size");

		auto bootCode = static_cast<const uint8_t*>(pbrCode);

		memcpy(&m_workArea[0], &bootCode[0], 3 + 8);
		memcpy(&m_workArea[0x3E], &bootCode[0x3E], 448);

		if (disk_write(m_driveNumber, m_workArea, firstBlock, 1) != RES_OK)
			throw std::runtime_error("failed to rewrite PBR");
	}

}

std::wstring FATFilesystem::pathToPartition(const std::wstring& path) {
	std::wstringstream stream;
	stream << m_driveNumber << L":/";
	if (!path.empty())
		stream << path;

	auto fpath = stream.str();

	for (wchar_t& ch : fpath) {
		if (ch == L'\\')
			ch = L'/';

	}

	return fpath;
}

bool FATFilesystem::createDirectory(const std::wstring& name) {
	auto path = pathToPartition(name);

	auto result = f_mkdir(path.c_str());
	if (result == FR_EXIST)
		return false;

	translateError(result);

	return true;
}

std::unique_ptr<IFile> FATFilesystem::open(const std::wstring& name, const std::wstring& mode) {
	return std::make_unique<FATFile>(this, pathToPartition(name), mode);
}

void FATFilesystem::setAttributes(const std::wstring& name, unsigned int attributes, unsigned int attributeMask) {
	translateError(f_chmod(name.c_str(), attributes, attributeMask));
}

FATFilesystem::FATFile::FATFile(FATFilesystem* parent, const std::wstring& name, const std::wstring& mode) : m_parent(parent) {
	static const std::unordered_map<std::wstring, int> modeMap{
		{ L"r", FA_READ },
		{ L"r+", FA_READ | FA_WRITE },
		{ L"w", FA_CREATE_ALWAYS | FA_WRITE },
		{ L"w+", FA_CREATE_ALWAYS | FA_WRITE | FA_READ },
		{ L"a", FA_OPEN_APPEND | FA_WRITE },
		{ L"a+", FA_OPEN_APPEND | FA_WRITE | FA_READ },
		{ L"wx", FA_CREATE_NEW | FA_WRITE },
		{ L"w+x", FA_CREATE_NEW | FA_WRITE | FA_READ }
	};

	auto it = modeMap.find(mode);
	if (it == modeMap.end())
		throw std::logic_error("unsupported mode");

	auto result = f_open(&m_file, name.c_str(), it->second);
	m_parent->translateError(result);
}

FATFilesystem::FATFile::~FATFile() {
	f_close(&m_file);
}		

int64_t FATFilesystem::FATFile::seek(int64_t offset, SeekWhence whence) {
	FSIZE_t target;

	switch (whence) {
	case SeekWhence::Begin:
		target = offset;
		break;

	case SeekWhence::Current:
		target = f_tell(&m_file) + offset;
		break;

	case SeekWhence::End:
		target = f_size(&m_file) + offset;
		break;

	default:
		throw std::logic_error("bad SeekWhence");
	}

	auto result = f_lseek(&m_file, target);
	m_parent->translateError(result);

	return target;
}

size_t FATFilesystem::FATFile::read(void* data, size_t size) {
	UINT read;

	auto result = f_read(&m_file, data, static_cast<UINT>(size), &read);
	m_parent->translateError(result);

	return  read;
}

size_t FATFilesystem::FATFile::write(const void* data, size_t size) {
	UINT written;

	auto result = f_write(&m_file, data, static_cast<UINT>(size), &written);
	m_parent->translateError(result);

	return written;
}

DWORD get_fattime(void) {
	auto timestamp = time(nullptr);
	tm parts;
	localtime_s(&parts, &timestamp);

	return
		(((parts.tm_year - 80) & 127) << 25) |
		(((parts.tm_mon + 1) & 15) << 21) |
		((parts.tm_mday & 31) << 16) |
		((parts.tm_hour & 31) << 11) |
		((parts.tm_min & 63) << 5) |
		((parts.tm_sec / 2) & 31);
}

void* ff_memalloc(UINT msize) {
	return malloc(msize);
}

void ff_memfree(void* mblock) {
	free(mblock);
}


DSTATUS disk_initialize(BYTE pdrv) {
	return disk_status(pdrv);
}

DSTATUS disk_status(BYTE pdrv) {
	auto fs = FATFilesystem::AllocatedDriveNumber::getFS(pdrv);

	if (fs == nullptr)
		return STA_NOINIT | STA_NODISK;

	return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
	auto fs = FATFilesystem::AllocatedDriveNumber::getFS(pdrv);

	if (fs == nullptr)
		return RES_NOTRDY;

	fs->m_storage->read(static_cast<uint64_t>(sector) * 512, buff, static_cast<size_t>(count) * 512);

	return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
	auto fs = FATFilesystem::AllocatedDriveNumber::getFS(pdrv);

	if (fs == nullptr)
		return RES_NOTRDY;

	fs->m_storage->write(static_cast<uint64_t>(sector) * 512, buff, static_cast<size_t>(count) * 512);

	return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
	auto fs = FATFilesystem::AllocatedDriveNumber::getFS(pdrv);

	if (fs == nullptr)
		return RES_NOTRDY;

	switch (cmd) {
	case CTRL_SYNC:
		fs->m_storage->flush();
		return RES_OK;

	case GET_SECTOR_COUNT:
	{
		auto dest = static_cast<LBA_t *>(buff);
		*dest = static_cast<LBA_t>(fs->m_storage->mediaSize() / 512);
		return RES_OK;
	}

	case GET_BLOCK_SIZE:
	{
		auto dest = static_cast<DWORD*>(buff);
		*dest = static_cast<DWORD>(fs->m_storage->allocationUnit() / 512);
		return RES_OK;
	}
	default:
		__debugbreak();
		return RES_PARERR;
	}
}

