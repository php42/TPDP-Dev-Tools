cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(TPDP-Dev-Tools C CXX CSharp)

# Override default install prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/bin" CACHE PATH "Default install path" FORCE)
endif()

# Compiler specific instruction set flags
# (mostly to allow compiling with clang + visual studio and -march=native)
if(MSVC)
    set(M_ARCH "SSE2" CACHE STRING "Extended instruction set to use, options are: SSE2 AVX AVX2")
else()
    set(M_ARCH "NATIVE" CACHE STRING "Extended instruction set to use, options are: SSE2 AVX AVX2 NATIVE")
endif()

# Enable /MP (parallel builds) in MSVC
if(${CMAKE_GENERATOR} MATCHES "Visual Studio")
    add_compile_options("/MP")
endif()

option(BUILD_CSHARP "Build csharp GUI front-ends" ON)
option(BUILD_LIBTPDP "Build libtpdp static library" ON)
option(BUILD_BINEDIT "Build binedit" ON)
option(BUILD_DIFFGEN "Build diffgen" ON)

if((BUILD_BINEDIT OR BUILD_DIFFGEN) AND NOT BUILD_LIBTPDP)
    message(FATAL_ERROR "Libtpdp is required for diffgen and binedit")
endif()

if(BUILD_BINEDIT OR BUILD_DIFFGEN)
    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/boost" CACHE PATH "Path to boost root directory")
    option(Boost_USE_STATIC_LIBS "Link against static Boost libs" ON)
    find_package(Boost REQUIRED COMPONENTS program_options)
    mark_as_advanced(CLEAR BOOST_ROOT)
endif()

if(BUILD_CSHARP)
    add_subdirectory(patcher)
    add_subdirectory(editor)
endif()

if(BUILD_LIBTPDP)
    add_subdirectory(libTPDP)
    install(FILES
    "common/typedefs.h"
    DESTINATION include/common)
    install(FILES
    "libTPDP/libtpdp.h"
    "libTPDP/archive.h"
    "libTPDP/gamedata.h"
    "libTPDP/puppet.h"
    "libTPDP/savefile.h"
    DESTINATION include/libTPDP)
endif()

if(BUILD_BINEDIT)
    add_subdirectory(binedit)
endif()

if(BUILD_DIFFGEN)
    # We don't need the vcdiff executable
    option(vcdiff_build_exec "Build vcdiff executable" OFF)
    mark_as_advanced(FORCE vcdiff_build_exec)
    add_subdirectory(deps/open-vcdiff)
    add_subdirectory(diffgen)
endif()

if(MSVC)
    # Suppress open-vcdiff compiler warnings and set config definitions
    set_property(DIRECTORY deps/open-vcdiff APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0601 UNICODE _UNICODE)
    if(BUILD_DIFFGEN)
        # Needed for open-vcdiff to compile as C++17
        target_compile_options(vcdcom_STATIC PRIVATE "/Zc:__cplusplus")
        target_compile_options(vcddec_STATIC PRIVATE "/Zc:__cplusplus")
        target_compile_options(vcdenc_STATIC PRIVATE "/Zc:__cplusplus")
    endif()
endif()
