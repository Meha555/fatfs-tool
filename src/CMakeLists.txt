set(getopt)
if(WIN32)
    find_package(unofficial-getopt-win32 REQUIRED)
    set(getopt unofficial::getopt-win32::getopt)
endif()

file(GLOB_RECURSE tool_srcs *.c)
add_executable(${PROJECT_NAME} ${tool_srcs})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE fatfs ${getopt})
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROGRAM_NAME="${PROJECT_NAME}"
)