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

# Allow static linking C runtime under MSVC
if(MSVC)
    option(STATIC_RUNTIME "Static link the C runtime" OFF)
    if(STATIC_RUNTIME)
        foreach(flag
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
            endif()
        endforeach()
        set(Boost_USE_STATIC_RUNTIME ON)
    endif()

    add_compile_options("$<$<CONFIG:Release>:/GL>" "$<$<CONFIG:Release>:/Oi>")
    add_link_options("$<$<CONFIG:Release>:/LTCG>")
endif()

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

    # Don't install zlib stuff
    set(SKIP_INSTALL_HEADERS ON)
    set(SKIP_INSTALL_FILES ON)
    add_subdirectory(deps/zlib)

    # Libzip config
    # Coerce find_package into using our cmake build of zlib
    set(ZLIB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib" CACHE PATH "Path to zlib root directory")
    set(ZLIB_LIBRARY zlib)
    find_package(ZLIB REQUIRED)
    OPTION(BUILD_TOOLS "Build tools in the src directory (zipcmp, zipmerge, ziptool)" OFF)
    OPTION(BUILD_REGRESS "Build regression tests" OFF)
    OPTION(BUILD_EXAMPLES "Build examples" OFF)
    OPTION(BUILD_DOC "Build documentation" OFF)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(deps/libzip)
    #set_property(DIRECTORY deps/libzip APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR}/deps/zlib)
    target_include_directories(zip PRIVATE ${CMAKE_BINARY_DIR}/deps/zlib)
    set_target_properties(dist distcheck PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    
    add_subdirectory(diffgen)
endif()

if(MSVC AND BUILD_DIFFGEN)
    # Suppress open-vcdiff compiler warnings and set config definitions
    set_property(DIRECTORY deps/open-vcdiff APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS _WIN32_WINNT=0x0601 WINVER=0x0601 NTDDI_VERSION=0x06010000 UNICODE _UNICODE)
    if(BUILD_DIFFGEN)
        # Needed for open-vcdiff to compile as C++17
        target_compile_options(vcdcom_STATIC PRIVATE "/Zc:__cplusplus")
        target_compile_options(vcddec_STATIC PRIVATE "/Zc:__cplusplus")
        target_compile_options(vcdenc_STATIC PRIVATE "/Zc:__cplusplus")
    endif()
endif()
