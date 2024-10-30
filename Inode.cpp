#include "Inode.h"
#include "IFilesystem.h"
#include "StringUtils.h"
#include "IFile.h"

#include <stdexcept>
#include <fstream>
#include <vector>

Inode::Inode(InodeType type, const std::string& name, Attributes attributes) : m_type(type), m_name(name), m_attributes(attributes) {

}

Inode::~Inode() = default;

std::shared_ptr<Inode> Inode::findExistingChildByName(const std::string& name) {
	auto inode = m_children.find(name);
	if (inode == m_children.end()) {
		throw std::runtime_error("child not found: " + name);
	}

	return inode->second;
}

std::shared_ptr<Inode> Inode::createNewChild(InodeType type, const std::string& name, Attributes attributes) {
	auto inode = std::make_shared<Inode>(type, name, attributes);
	auto result = m_children.emplace(name, inode);

	if (!result.second)
		throw std::runtime_error("child already exists: " + name);

	return inode;
}

size_t Inode::calculateSize(size_t clusterSizeBytes) const {
	if (m_type == InodeType::Directory) {
		size_t entries;

		if (m_name.empty()) {
			entries = 512;
		}
		else {
			entries = m_children.size();
		}

		auto size = entries * 32;
		size = (size + clusterSizeBytes - 1) & ~(clusterSizeBytes - 1);

		for (const auto& child : m_children) {
			size += child.second->calculateSize(clusterSizeBytes);
		}

		return size;
	}
	else {
		auto size = std::filesystem::file_size(m_sourceFileName);
		size = (size + clusterSizeBytes - 1) & ~(clusterSizeBytes - 1);

		return size;
	}
}

void Inode::buildFilesystem(IFilesystem* fs, const std::string& pathPrefix) {
	auto fullPath = pathPrefix + "/" + m_name;
	auto fullPathUnicode = utf8StringToFatfsString(fullPath);

	if (m_type == InodeType::Directory) {
		if (!m_name.empty()) {
			fs->createDirectory(fullPathUnicode);
		}

		for (const auto& child : m_children) {
			child.second->buildFilesystem(fs, fullPath);
		}
	}
	else {
		auto file = fs->open(fullPathUnicode, FF_T("w"));

		std::ifstream source;
		source.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
		source.open(m_sourceFileName, std::ios::in | std::ios::binary);
		source.exceptions(std::ios::badbit);

		std::vector<char> buf(8192);
		size_t bytesTransferred;
		do {
			source.read(buf.data(), buf.size());
			bytesTransferred = source.gcount();

			file->write(buf.data(), bytesTransferred);

		} while (bytesTransferred == buf.size());
	}

	if (m_attributes != AttributeDefault) {
		fs->setAttributes(fullPathUnicode, m_attributes, AttributeArchive | AttributeSystem | AttributeHidden | AttributeReadOnly);
	}
}

void Inode::enumerateInputs(const std::function<void(const std::filesystem::path&)>& func) const {
	if (m_type == InodeType::Directory) {
		for (const auto& child : m_children) {
			child.second->enumerateInputs(func);
		}
	}
	else {
		func(m_sourceFileName);
	}
}
