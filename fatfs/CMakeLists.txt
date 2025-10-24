cmake_minimum_required(VERSION 3.10)

project(fatfs
    LANGUAGES C
    HOMEPAGE_URL https://www.elm-chan.org/fsw/ff
    VERSION 0.16.0
)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

set(PORT_BACKEND "file")

if(NOT PORT_BACKEND)
    message(FATAL_ERROR "Please specify PORT_BACKEND")
endif()

add_library(fatfs
    ff.c
    ffsystem.c
    ffunicode.c
    ports/port.c
    ports/${PORT_BACKEND}/diskio.c
)
target_include_directories(fatfs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ports
    ${CMAKE_CURRENT_SOURCE_DIR}/ports/${PORT_BACKEND}
)
target_compile_definitions(fatfs PRIVATE
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>:OS_TYPE=0>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},ITRON>:OS_TYPE=1>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},uCOS>:OS_TYPE=2>
    $<$<STREQUAL:${CMAKE_SYSTEM_NAME},FreeRTOS>:OS_TYPE=3>
    $<$<OR:$<STREQUAL:${CMAKE_SYSTEM_NAME},CMSIS-RTOS>,$<STREQUAL:${CMAKE_SYSTEM_NAME},Generic>>:OS_TYPE=4>
    $<$<NOT:$<OR:$<STREQUAL:${CMAKE_SYSTEM_NAME},Windows>,$<STREQUAL:${CMAKE_SYSTEM_NAME},ITRON>,$<STREQUAL:${CMAKE_SYSTEM_NAME},uCOS>,$<STREQUAL:${CMAKE_SYSTEM_NAME},FreeRTOS>,$<STREQUAL:${CMAKE_SYSTEM_NAME},CMSIS-RTOS>,$<STREQUAL:${CMAKE_SYSTEM_NAME},Generic>>>:OS_TYPE=0>
)

target_compile_definitions(fatfs PUBLIC VIRTUAL_DISK_FAT16)