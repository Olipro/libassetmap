cmake_minimum_required(VERSION 3.17)
project(libassetmap
    VERSION
        1.0.0
    DESCRIPTION
        "Hash-table based archive format containing compressed data"
    HOMEPAGE_URL
        https://github.com/Olipro/libassetmap)

add_subdirectory(ext/zstd)

set(LIBARCHIVEMAP_SIZE_TYPE uint32_t CACHE STRING "type to use for object sizes. uint32_t (default) or uint64_t suggested.")

include(ext/CityHash.cmake)

add_library(libassetmap OBJECT
    include/IHasher.h
    include/MemOps.h
    ext/cityhash/src/city.cc ext/cityhash/src/city.h
    src/MemMappedBucket.cpp include/MemMappedBucket.h
    src/MemMappedBucketEntry.cpp include/MemMappedBucketEntry.h
    src/Hashers.cpp include/Hashers.h
    src/ZSTDComp.cpp include/ZSTDComp.h
    src/DirectoryMetadata.cpp include/DirectoryMetadata.h
    src/MemMappedArchive.cpp include/MemMappedArchive.h include/ICompress.h include/IDecompress.h)
target_sources(libassetmap
    INTERFACE
        $<TARGET_OBJECTS:zstd>)
target_compile_definitions(libassetmap
    PRIVATE
        $<TARGET_PROPERTY:zstd,INTERFACE_COMPILE_DEFINITIONS>
    PUBLIC
        LIBARCHIVEMAP_SIZE_TYPE=${LIBARCHIVEMAP_SIZE_TYPE}
        ZSTD_STATIC_LINKING_ONLY
        ZDICT_STATIC_LINKING_ONLY)
if (UNIX)
    set(PRIVATE_SOURCES src/posix/MemMapper.cpp include/posix/MemMapper.h)
    set(PUBLIC_INCLUDES include/posix)
elseif (WIN32)
    set(PRIVATE_SOURCES src/win/MemMapper.cpp include/win/MemMapper.h)
    set(PUBLIC_INCLUDES include/win)
endif()
target_sources(libassetmap
    PRIVATE
        ${PRIVATE_SOURCES})
target_include_directories(libassetmap
    PRIVATE
        ext/cityhash/src
        ext/zstd/lib
        ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC
        ${PUBLIC_INCLUDES})
target_include_directories(libassetmap
    PUBLIC
        include)

find_package(Doxygen OPTIONAL_COMPONENTS dot)
if (DOXYGEN_FOUND)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_EXCLUDE_PATTERNS */include/win/*)
    doxygen_add_docs(libassetmap_doc
        ${PROJECT_SOURCE_DIR}/README.md
        ${PROJECT_SOURCE_DIR}/include)
endif()

file(DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v1.9.1/CLI11.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/CLI11.hpp
    EXPECTED_HASH
        SHA256=52a3bc829a000fb26645328c9093b014e08547dde50c66d4316aef624046dc4b)

add_executable(assetmapcli
    $<TARGET_OBJECTS:libassetmap>
    $<TARGET_PROPERTY:libassetmap,INTERFACE_SOURCES>
    AssetMapCLI.cpp)
target_include_directories(assetmapcli
    PRIVATE
        $<TARGET_PROPERTY:libassetmap,INTERFACE_INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_BINARY_DIR}
)
target_precompile_headers(assetmapcli
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/CLI11.hpp
        <filesystem>)
target_compile_definitions(assetmapcli
    PRIVATE
        $<TARGET_PROPERTY:libassetmap,INTERFACE_COMPILE_DEFINITIONS>)

file(DOWNLOAD https://github.com/catchorg/Catch2/releases/download/v2.13.3/catch.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
    EXPECTED_HASH
        SHA256=fd043a34e9889c4e91b9133ce8efcbce9d11970b266c1d7b81d62087a92fa6cb)
add_executable(testarchive EXCLUDE_FROM_ALL
        $<TARGET_OBJECTS:libassetmap>
        $<TARGET_PROPERTY:libassetmap,INTERFACE_SOURCES>
        ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp
        test/TestMain.cpp
        test/TestArchive.cpp)
target_include_directories(testarchive
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        $<TARGET_PROPERTY:libassetmap,INTERFACE_INCLUDE_DIRECTORIES>)
target_compile_definitions(testarchive
    PRIVATE
        $<TARGET_PROPERTY:libassetmap,INTERFACE_COMPILE_DEFINITIONS>)

set_target_properties(libassetmap assetmapcli testarchive
    PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES)

install(TARGETS assetmapcli
    RUNTIME DESTINATION bin)
