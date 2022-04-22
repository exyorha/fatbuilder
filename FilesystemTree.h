#ifndef FILESYSTEM_TREE_H
#define FILESYSTEM_TREE_H

#include <filesystem>
#include <ios>
#include <vector>
#include <string>

#include "Inode.h"

class IFilesystem;

class FilesystemTree {
public:
	FilesystemTree();
	~FilesystemTree();

	FilesystemTree(const FilesystemTree& other) = delete;
	FilesystemTree &operator =(const FilesystemTree& other) = delete;

	void parse(const std::filesystem::path& path);
	void parse(std::istream& stream);

	size_t calculateSize(size_t clusterSizeBytes, size_t additionalFreeSpace) const;

	void buildFilesystem(IFilesystem* fs);

private:
	void processLine(const std::vector<std::string>& line);
	Attributes parseAttributes(const std::string& attrs);
	std::shared_ptr<Inode> createInode(InodeType type, const std::string& name, Attributes attributes);

	std::shared_ptr<Inode> m_root;
};

#endif
