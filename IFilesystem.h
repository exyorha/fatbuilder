#ifndef FILESYSTEM_IFILESYSTEM_H
#define FILESYSTEM_IFILESYSTEM_H

#include <string>
#include <memory>
#include "StringUtils.h"

class IFile;

class IFilesystem {
protected:
	IFilesystem();

public:
	virtual ~IFilesystem();

	IFilesystem(const IFilesystem& other) = delete;
	IFilesystem &operator =(const IFilesystem& other) = delete;

	virtual bool createDirectory(const FatfsString& name) = 0;
	virtual std::unique_ptr<IFile> open(const FatfsString& name, const std::string& mode) = 0;
	virtual void setAttributes(const FatfsString& name, unsigned int attributes, unsigned int attributeMask) = 0;
};

#endif
