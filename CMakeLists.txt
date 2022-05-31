cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0094)
    cmake_policy(SET CMP0094 NEW)
endif()

if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW) # option() honors variables already set
endif()

if(DEFINED ENV{MAVLINK_DIALECT})
    set(MAVLINK_DIALECT "$ENV{MAVLINK_DIALECT}")
else()
    set(MAVLINK_DIALECT common)
endif()

option(USE_SYSTEM_MAVLINK "Use system-wide installed mavlink headers" OFF)
option(MAVKIT_ENABLE_LTO "Enable support for link-time optimization" ON)

if(USE_SYSTEM_MAVLINK)
    set(MAVLINK_INCLUDES /usr/include/mavlink)
else()
    set(MAVLINK_INCLUDES ${CMAKE_SOURCE_DIR}/ext/c_library_v2)
endif()

project(
    mavkit
    LANGUAGES CXX
    VERSION 0.0.1
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    set(CLANG_DEFAULT_CXX_STDLIB libc++)
    set(CLANG_DEFAULT_RTLIB compiler-rt)
endif()

set(CMAKE_DEBUG_POSTFIX d)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "Default build type: Release" FORCE
    )
endif()

# LTO requires cmake min version 3.9 and clang LTO requires lld
if(MAVKIT_ENABLE_LTO AND CMAKE_BUILD_TYPE MATCHES "Release")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        find_package(LLVM CONFIG)
    endif()

    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT error)

    if(ipo_supported)
        message(STATUS "IPO / LTO supported")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(WARNING "IPO / LTO not supported: <${error}>")
    endif()
endif()

include(CTest)
include(GNUInstallDirs)

# CONFIGURE
configure_file(mavlink_dialect.h.in mavlink_dialect.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_compile_options(
    -Wno-address-of-packed-member -Wno-cast-align  # suppress mavlink warnings
)

# PACKAGES
find_package(Boost REQUIRED COMPONENTS regex)
find_package(Threads REQUIRED)

# INCLUDES
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${MAVLINK_INCLUDES}
)

# SOURCES
file(GLOB_RECURSE MAVKIT_SRC "src/mavkit/*.cpp")

# LINK
LIST(APPEND LINK_LIBS
	${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

# EXECUTABLE
add_executable(${PROJECT_NAME} ${MAVKIT_SRC} src/mavkit.cpp)
target_link_libraries(${PROJECT_NAME} ${LINK_LIBS})
