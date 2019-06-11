cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libTPDP)

if(MSVC)
    if(M_ARCH STREQUAL "SSE2")
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            add_compile_options("/arch:SSE2")
        endif()
        add_compile_definitions(ARC_NO_AVX)
    elseif(M_ARCH STREQUAL "AVX")
        add_compile_options("/arch:AVX")
        add_compile_definitions(ARC_NO_AVX)
    elseif(M_ARCH STREQUAL "AVX2")
        add_compile_options("/arch:AVX2")
    endif()
else()
    if(M_ARCH STREQUAL "SSE2")
        add_compile_options("-msse2")
        add_compile_definitions(ARC_NO_AVX)
    elseif(M_ARCH STREQUAL "AVX")
        add_compile_options("-mavx")
        add_compile_definitions(ARC_NO_AVX)
    elseif(M_ARCH STREQUAL "AVX2")
        add_compile_options("-mavx2")
    elseif(M_ARCH STREQUAL "NATIVE")
        add_compile_options("-march=native")
    endif()
endif()

add_library(libtpdp STATIC
    "archive.h"
    "archive.cpp"
    "gamedata.h"
    "gamedata.cpp"
    "libtpdp.h"
    "puppet.h"
    "puppet.cpp"
    "savefile.h"
    "savefile.cpp"
    "../common/endian.h"
    "../common/filesystem.h"
    "../common/filesystem.cpp"
    "../common/textconvert.h"
    "../common/textconvert.cpp"
    "../common/typedefs.h"
    "../common/version.h"
)

target_include_directories(libtpdp PUBLIC ".")
target_include_directories(libtpdp PRIVATE "../common")

# Specify Windows 7 compatibility
target_compile_definitions(libtpdp PRIVATE _WIN32_WINNT=0x0601)

if(MSVC)
    target_compile_options(libtpdp PRIVATE "/W4")
else()
    target_compile_options(libtpdp PRIVATE "-Wall")
endif()
