add_library(fatfs STATIC 
	diskio.h
	ff.c
	ff.h
	ffconf.h
	ffunicode.c
)
target_include_directories(fatfs PUBLIC .)
target_compile_definitions(fatfs PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
