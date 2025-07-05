cmake_minimum_required(VERSION 3.31)
project(konfig)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

string(APPEND CMAKE_CXX_FLAGS_RELEASE " -DNDEBUG")
string(APPEND CMAKE_C_FLAGS_RELEASE " -DNDEBUG")

option(BUILD_SHARED_LIBS "Build shared libraries instead of static ones" OFF)

add_library(konfig konfig.cpp konfig.h)
target_include_directories(konfig PUBLIC thirdparty)

add_subdirectory(tests)
