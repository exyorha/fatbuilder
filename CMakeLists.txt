cmake_minimum_required(VERSION 3.20)
project(fatbuilder)

add_subdirectory(3rdparty)

add_executable(fatbuilder
	FATFilesystem.cpp
	FATFilesystem.h
	FilesystemTree.cpp
	FilesystemTree.h
	IBlockDevice.cpp
	IBlockDevice.h
	IFile.cpp
	IFile.h
	IFilesystem.cpp
	IFilesystem.h
	Inode.cpp
	Inode.h
	main.cpp
	RawBlockDevice.cpp
	RawBlockDevice.h
	StringUtils.h
)
target_link_libraries(fatbuilder PRIVATE CLI11::CLI11 fatfs)
set_target_properties(fatbuilder PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)
target_compile_definitions(fatbuilder PRIVATE -DUNICODE -D_UNICODE -D_FILE_OFFSET_BITS=64)
if(WIN32)
	target_compile_definitions(fatbuilder PRIVATE -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_VC_EXTRALEAN)
	target_sources(fatbuilder PRIVATE
		StringUtils.cpp
	)
endif()
