cmake_minimum_required(VERSION 3.10)
project(teik_index)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

if(NOT APPLE)
    find_package(OpenSSL REQUIRED)
    # target_include_directories(teik_index PRIVATE /opt/homebrew/opt/openssl@3/include)
endif()

# Find ZLIB
find_package(ZLIB REQUIRED)

# Find LibMagic
find_path(MAGIC_INCLUDE_DIR magic.h)
find_library(MAGIC_LIBRARY NAMES magic)

# Find Zip
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED IMPORTED_TARGET libzip)

# Find sources
file(GLOB SOURCES "src/*.c")

# Include the headers
include_directories(${MAGIC_INCLUDE_DIR})

add_executable(teik_index ${SOURCES})

target_link_libraries(teik_index PRIVATE PkgConfig::LIBZIP ZLIB::ZLIB ${MAGIC_LIBRARY})

if(NOT APPLE)
    target_link_libraries(teik_index PRIVATE OpenSSL::SSL)
endif()