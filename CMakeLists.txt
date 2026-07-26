# The MIT License (MIT)
#
# Copyright (c) 2021-2024 Alexander Kurbatov
# (in particular the cpp_sc2.cmake Module for fetching the sc2 repo)

cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(s3b VERSION 0.0.1 LANGUAGES CXX)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify output directories
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_LIST_DIR}/bin")

if (BUILD_FOR_DIST)
    set(FETCHCONTENT_BASE_DIR "/cache/fetchcontent")
    set(protobuf_BUILD_TESTS OFF)
endif ()

if (MSVC)
    # Setup MSVC parallelized builds
    add_compile_options(/MP)

    # Use statically linked runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# cpp-sc2 dependency
include(cpp_sc2)

set(CMAKE_INCLUDE_CURRENT_DIR true)
add_executable(s3b 
    src/bot_kq.hh
    src/bot_kq.cpp

    src/unit_data.h
    src/utils.h

    src/main.cpp
)

if (NOT MSVC)
    target_compile_options(s3b PRIVATE -Wall -Wextra)

else()
    target_compile_options(s3b PRIVATE /MP /W4 /WX)
endif()

# TODO: Figure out how to do this properly, this cant be right way
target_include_directories(s3b PRIVATE
    ${cpp_sc2_SOURCE_DIR}/src/sc2api
    ${cpp_sc2_SOURCE_DIR}/src/sc2lib
    ${cpp_sc2_SOURCE_DIR}/src/sc2protocol
    ${cpp_sc2_SOURCE_DIR}/src/sc2utils
)

target_link_libraries(s3b PRIVATE sc2lib sc2api sc2protocol sc2utils)