#include "FilesystemTree.h"

#include <fstream>
#include <unordered_map>
#include <string_view>

FilesystemTree::FilesystemTree() : m_root(std::make_shared<Inode>(InodeType::Directory, "", AttributeDefault)) {

}

FilesystemTree::~FilesystemTree() = default;

void FilesystemTree::parse(const std::filesystem::path & path) {
	std::ifstream stream;
	stream.exceptions(std::ios::failbit | std::ios::eofbit | std::ios::badbit);
	stream.open(path, std::ios::in | std::ios::binary);
	stream.exceptions(std::ios::badbit);

	parse(stream);
}

void FilesystemTree::parse(std::istream& stream) {
	enum {
		Normal,
		String,
		Escaped,
		Comment
	} lexerState = Normal;
	std::vector<std::string> tokens;
	std::string tokenBuffer;

	char character;
	bool tokenBufferActive = false;

	while (true) {
		stream.get(character);

		if (stream.fail())
			break;

		switch (lexerState) {
		case Normal:
			if (character == '"') {
				tokenBufferActive = true;
				lexerState = String;
			}
			else if (character == ';') {
				lexerState = Comment;
			}
			else if (isspace((unsigned char)character)) {
				if (tokenBufferActive) {
					tokens.push_back(tokenBuffer);
					tokenBuffer.clear();
					tokenBufferActive = false;
				}

				if (character == '\n' && tokens.size() != 0) {
					processLine(tokens);
					tokens.clear();
				}
			}
			else {
				tokenBuffer.push_back(character);
				tokenBufferActive = true;
			}

			break;

		case String:
			if (character == '\\')
				lexerState = Escaped;
			else if (character == '"')
				lexerState = Normal;
			else
				tokenBuffer.push_back(character);

			break;

		case Escaped:
			tokenBuffer.push_back(character);
			lexerState = String;

			break;

		case Comment:
			if (character == '\n') {
				if (tokenBufferActive) {
					tokens.push_back(tokenBuffer);
					tokenBuffer.clear();
					tokenBufferActive = false;
				}

				if (tokens.size() != 0) {
					processLine(tokens);
					tokens.clear();
				}

				lexerState = Normal;
			}

			break;
		}
	}

	if (lexerState != Normal)
		throw std::runtime_error("End of file reached before closing quote");

	if (tokenBufferActive || !tokens.empty())
		throw std::runtime_error("No newline at the end of file");
}

void FilesystemTree::processLine(const std::vector<std::string>& line) {
	static const std::unordered_map<std::string, InodeType> inodeTypes{
		{ "file", InodeType::File },
		{ "dir",  InodeType::Directory }
	};

	InodeType type;

	auto it = line.begin();

	if (it == line.end()) {
		throw std::runtime_error("no inode type is specified");
	}

	auto inodeIt = inodeTypes.find(*it);
	if (inodeIt == inodeTypes.end()) {
		throw std::runtime_error("unsupported inode type: " + *it);
	}
	type = inodeIt->second;

	++it;

	if (it == line.end()) {
		throw std::runtime_error("no inode name is specified");
	}

	auto name = *it;

	++it;

	std::filesystem::path sourceFileName;

	if (type == InodeType::File) {
		if (it == line.end()) {
			throw std::runtime_error("no source file name is specified");
		}

		sourceFileName = *it;
		++it;
	}

	Attributes attributes = AttributeDefault;

	if (it != line.end()) {
		attributes = parseAttributes(*it);
		++it;
	}

	auto inode = createInode(type, name, attributes);

	if (type == InodeType::File) {
		inode->setSourceFileName(std::move(sourceFileName));
	}
}

Attributes FilesystemTree::parseAttributes(const std::string& attrs) {
	Attributes attributes = 0;

	for (auto attribute : attrs) {
		switch (attribute) {
		case 'a':
			attributes |= AttributeArchive;
			break;

		case 's':
			attributes |= AttributeSystem;
			break;

		case 'h':
			attributes |= AttributeHidden;
			break;

		case 'r':
			attributes |= AttributeReadOnly;
			break;

		default:
			throw std::runtime_error("unsupported attributes: " + attrs);
		}
	}

	return attributes;
}

std::shared_ptr<Inode> FilesystemTree::createInode(InodeType type, const std::string& name, Attributes attributes) {
	auto directory = m_root;
	size_t pos = 0;

	while (true) {
		if (directory->type() != InodeType::Directory)
			throw std::runtime_error("not a directory in path: " + name);

		auto terminator = name.find('/', pos);

		auto thisName = name.substr(pos, terminator - pos);

		if (terminator == std::string::npos) {
			return directory->createNewChild(type, thisName, attributes);
		}
		else {
			directory = directory->findExistingChildByName(thisName);

			pos = terminator + 1;
		}
	}
}

size_t FilesystemTree::calculateSize(size_t clusterSizeBytes, size_t additionalFreeSpace) const {
	auto size = m_root->calculateSize(clusterSizeBytes) + ((additionalFreeSpace + (clusterSizeBytes - 1)) & ~(clusterSizeBytes - 1));

	auto clusters = size / clusterSizeBytes;

	bool isFat32 = clusters > 65525;
	size_t fatEntrySize;

	if (isFat32) {
		fatEntrySize = 4;
	}
	else {
		fatEntrySize = 2;
	}

	auto fatSizeSectors = 2 * ((clusters * fatEntrySize + 511) / 511);

	size_t totalSizeSectors = clusters * clusterSizeBytes / 512 + fatSizeSectors;
	totalSizeSectors += 72;

	return totalSizeSectors * 512;
}

void FilesystemTree::buildFilesystem(IFilesystem* fs) {
	m_root->buildFilesystem(fs, "");
}
