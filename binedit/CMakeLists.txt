cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TPDPbinedit)

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

add_executable(binedit
    "binedit.h"
    "binedit.cpp"
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

target_link_libraries(binedit PRIVATE libtpdp Boost::boost Boost::program_options)
target_include_directories(binedit PRIVATE "../common")

# Specify Windows 7 compatibility
target_compile_definitions(binedit PRIVATE _WIN32_WINNT=0x0601)

if(MSVC)
    target_compile_options(binedit PRIVATE "/W4")
else()
    target_compile_options(binedit PRIVATE "-Wall")
endif()
