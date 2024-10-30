#ifndef INODE_H
#define INODE_H

#include <string>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <functional>

class IFilesystem;

enum class InodeType {
	File,
	Directory
};

typedef unsigned int Attributes;

static constexpr Attributes AttributeArchive  = 1 << 5;
static constexpr Attributes AttributeReadOnly = 1 << 0;
static constexpr Attributes AttributeHidden  = 1 << 1;
static constexpr Attributes AttributeSystem  = 1 << 2;
static constexpr Attributes AttributeDefault = AttributeArchive;

class Inode {
public:
	Inode(InodeType type, const std::string &name, Attributes attributes);
	~Inode();
	
	Inode(const Inode& other) = delete;
	Inode &operator =(const Inode& other) = delete;

	inline InodeType type() const {
		return m_type;
	}

	inline const std::string& name() const {
		return m_name;
	}

	inline Attributes attributes() const {
		return m_attributes;
	}

	std::shared_ptr<Inode> findExistingChildByName(const std::string& name);
	std::shared_ptr<Inode> createNewChild(InodeType type, const std::string& name, Attributes attributes);

	inline const std::filesystem::path& sourceFileName() const {
		return m_sourceFileName;
	}

	inline void setSourceFileName(const std::filesystem::path& filename) {
		m_sourceFileName = filename;
	}

	inline void setSourceFileName(std::filesystem::path&& filename) {
		m_sourceFileName = std::move(filename);
	}

	size_t calculateSize(size_t clusterSizeBytes) const;

	void buildFilesystem(IFilesystem* fs, const std::string& pathPrefix);

	void enumerateInputs(const std::function<void(const std::filesystem::path&)>& func) const;

private:
	InodeType m_type;
	std::string m_name;
	Attributes m_attributes;
	std::unordered_map<std::string, std::shared_ptr<Inode>> m_children;
	std::filesystem::path m_sourceFileName;
};

#endif
