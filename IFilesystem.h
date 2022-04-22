#ifndef FILESYSTEM_IFILESYSTEM_H
#define FILESYSTEM_IFILESYSTEM_H

#include <string>
#include <memory>

class IFile;

class IFilesystem {
protected:
	IFilesystem();

public:
	virtual ~IFilesystem();

	IFilesystem(const IFilesystem& other) = delete;
	IFilesystem &operator =(const IFilesystem& other) = delete;

	virtual bool createDirectory(const std::wstring& name) = 0;
	virtual std::unique_ptr<IFile> open(const std::wstring& name, const std::wstring& mode) = 0;
	virtual void setAttributes(const std::wstring& name, unsigned int attributes, unsigned int attributeMask) = 0;
};

#endif
