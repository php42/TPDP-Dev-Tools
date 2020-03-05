cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TPDPdiffgen)

if(MSVC)
    if(M_ARCH STREQUAL "SSE2" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_compile_options("/arch:SSE2")
    elseif(M_ARCH STREQUAL "AVX")
        add_compile_options("/arch:AVX")
    elseif(M_ARCH STREQUAL "AVX2")
        add_compile_options("/arch:AVX2")
    endif()
else()
    if(M_ARCH STREQUAL "SSE2")
        add_compile_options("-msse2")
    elseif(M_ARCH STREQUAL "AVX")
        add_compile_options("-mavx")
    elseif(M_ARCH STREQUAL "AVX2")
        add_compile_options("-mavx2")
    elseif(M_ARCH STREQUAL "NATIVE")
        add_compile_options("-march=native")
    endif()
endif()

add_executable(diffgen
    "legacy.h"
    "legacy.cpp"
    "diffgen.h"
    "diffgen.cpp"
    "ziparchive.h"
    "ziparchive.cpp"
    "main.cpp"
    "../common/console.h"
    "../common/endian.h"
    "../common/filesystem.h"
    "../common/filesystem.cpp"
    "../common/textconvert.h"
    "../common/textconvert.cpp"
    "../common/typedefs.h"
    "../common/version.h"
)

target_link_libraries(diffgen PRIVATE libtpdp zlib zip vcdcom vcddec vcdenc Boost::boost Boost::program_options)
target_include_directories(diffgen PRIVATE
    "../common"
    "../deps/open-vcdiff/src"
    "../deps/zlib"
    "../deps/libzip/lib"
    "${CMAKE_BINARY_DIR}/deps/zlib"
    ${CMAKE_BINARY_DIR}/deps/libzip)

# Specify Windows 7 compatibility and unicode support
target_compile_definitions(diffgen PRIVATE _WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000 UNICODE _UNICODE)

if(MSVC)
    target_compile_options(diffgen PRIVATE "/W4")
else()
    target_compile_options(diffgen PRIVATE "-Wall")
endif()

install(TARGETS diffgen)
