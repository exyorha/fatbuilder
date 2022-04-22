#include <CLI/CLI.hpp>

#include "FilesystemTree.h"
#include "RawBlockDevice.h"
#include "FATFilesystem.h"

int main(int argc, char** argv) {
	CLI::App app("FAT filesystem builder", "fatbuilder");

	std::filesystem::path inputFilename;
	std::filesystem::path outputFilename;

	app.add_option("--input", inputFilename)->required(true);
	app.add_option("--output", outputFilename)->required(true);

	CLI11_PARSE(app, argc, argv);

	FilesystemTree tree;
	tree.parse(inputFilename);

	auto size = tree.calculateSize(32768, 1024 * 1024);

	auto blockDevice = std::make_unique<RawBlockDevice>(std::move(outputFilename), size);
	auto fs = std::make_unique<FATFilesystem>(std::move(blockDevice));

	tree.buildFilesystem(fs.get());

	return 0;
}