#ifndef FILESYSTEM_IFILE_H
#define FILESYSTEM_IFILE_H

#include <string.h>
#include <stdint.h>

class IFile {
protected:
	IFile();

public:
	virtual ~IFile();

	IFile(const IFile& other) = delete;
	IFile &operator =(const IFile& other) = delete;

	enum class SeekWhence {
		Begin,
		Current,
		End
	};

	virtual int64_t seek(int64_t offset, SeekWhence whence) = 0;
	virtual size_t read(void* data, size_t size) = 0;
	virtual size_t write(const void* data, size_t size) = 0;
};

#endif
