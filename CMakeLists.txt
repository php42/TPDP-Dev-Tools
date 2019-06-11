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

set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/boost" CACHE PATH "Path to boost root directory")
option(Boost_USE_STATIC_LIBS "Link against static Boost libs" ON)
find_package(Boost REQUIRED COMPONENTS program_options)
mark_as_advanced(CLEAR BOOST_ROOT)

# We don't need the vcdiff executable
option(vcdiff_build_exec "Build vcdiff executable" OFF)
mark_as_advanced(FORCE vcdiff_build_exec)

add_subdirectory(deps/open-vcdiff)
add_subdirectory(patcher)
add_subdirectory(libTPDP)
add_subdirectory(binedit)
add_subdirectory(diffgen)

if(MSVC)
    # Suppress open-vcdiff compiler warnings
    set_property(DIRECTORY deps/open-vcdiff APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
    # Needed for open-vcdiff to compile as C++17
    target_compile_options(vcdcom_STATIC PRIVATE "/Zc:__cplusplus")
    target_compile_options(vcddec_STATIC PRIVATE "/Zc:__cplusplus")
    target_compile_options(vcdenc_STATIC PRIVATE "/Zc:__cplusplus")
endif()
