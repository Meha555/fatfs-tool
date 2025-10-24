cmake_minimum_required(VERSION 3.10)

project(fatfs-tool
    VERSION 0.1.0
    LANGUAGES C
)

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(fatfs)
add_subdirectory(src)