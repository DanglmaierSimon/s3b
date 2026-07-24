# The MIT License (MIT)
#
# Copyright (c) 2021-2024 Alexander Kurbatov

cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(FETCHCONTENT_FULLY_DISCONNECTED TRUE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(s3b)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

option(BUILD_FOR_LADDER "Create build for the AIArena ladder" ON)

# Build with c++17 support, required by sc2api
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify output directories
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/lib")

if (MSVC)
    # Setup MSVC parallelized builds
    add_compile_options(/MP)

    # Use statically linked runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# cpp-sc2 dependency
include(cpp_sc2)

# bot sources
# add_subdirectory(s3b)



set(CMAKE_INCLUDE_CURRENT_DIR true)
add_executable(s3b 
    src/bot_examples.cpp
    src/bot_examples.h
    src/bot_kq.cpp
    src/bot_kq.hh
    src/main.cpp
)

target_include_directories(s3b PUBLIC
    ${cpp_sc2_SOURCE_DIR}/src/sc2api
    ${cpp_sc2_SOURCE_DIR}/src/sc2lib
    ${cpp_sc2_SOURCE_DIR}/src/sc2protocol
    ${cpp_sc2_SOURCE_DIR}/src/sc2utils
)

target_link_libraries(s3b PUBLIC sc2lib sc2api sc2protocol sc2utils)

if (MINGW)
elseif (APPLE)
# Building on Linux
elseif (UNIX AND NOT APPLE)

endif ()