#include <CLI/CLI.hpp>

#include "FATFilesystemLayout.h"
#include "FilesystemTree.h"
#include "RawBlockDevice.h"
#include "FATFilesystem.h"

static std::unique_ptr<unsigned char[]> loadCodeFile(const std::filesystem::path& path, size_t size) {
	std::ifstream stream;
	stream.exceptions(std::ios::failbit | std::ios::eofbit | std::ios::badbit);
	stream.open(path, std::ios::in | std::ios::binary);

	auto result = std::make_unique<unsigned char[]>(size);

	stream.read(reinterpret_cast<char*>(result.get()), size);

	return result;
}

int main(int argc, char** argv) {
	CLI::App app("FAT filesystem builder", "fatbuilder");

	std::filesystem::path inputFilename;
	std::filesystem::path outputFilename;

	FATFilesystemLayout layout;

	std::unique_ptr<unsigned char[]> mbrCode;
	std::unique_ptr<unsigned char[]> pbrCode12_16;
	std::unique_ptr<unsigned char[]> pbrCode32;

	app.add_option("--input", inputFilename)->required(true);
	app.add_option("--output", outputFilename)->required(true);

	app.add_option_function<std::filesystem::path>("--mbr-code", [&layout, &mbrCode](const std::filesystem::path& path) {
		mbrCode = loadCodeFile(path, FATFilesystemLayout::MBRCodeSize);
		layout.mbrCode = mbrCode.get();
	});

	app.add_option_function<std::filesystem::path>("--pbr-code1216", [&layout, &pbrCode12_16](const std::filesystem::path& path) {
		pbrCode12_16 = loadCodeFile(path, FATFilesystemLayout::PBRCode12_16Size);
		layout.pbrCode12_16 = pbrCode12_16.get();
	});

	app.add_option_function<std::filesystem::path>("--pbr-code32", [&layout, &pbrCode32](const std::filesystem::path& path) {
		pbrCode32 = loadCodeFile(path, FATFilesystemLayout::PBRCode32Size);
		layout.pbrCode32 = pbrCode32.get();
	});

	CLI11_PARSE(app, argc, argv);

	FilesystemTree tree;
	tree.parse(inputFilename);

	auto size = tree.calculateSize(32768, 1024 * 1024);

	auto blockDevice = std::make_unique<RawBlockDevice>(std::move(outputFilename), size);
	auto fs = std::make_unique<FATFilesystem>(std::move(blockDevice), layout);

	tree.buildFilesystem(fs.get());

	return 0;
}
