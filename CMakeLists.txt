# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
#
# This file is part of EmbLink Studio, the IDE for EmbLinkRTOS.
# Licensed under the Apache License, Version 2.0; see the LICENSE file at the repository root.
#
# Top-level build: the EmbLink Studio desktop (Qt 6, C++17) and its tests.
# Configure with a preset from CMakePresets.json, e.g. `cmake --preset dev`.
cmake_minimum_required(VERSION 3.21)

project(EmbLinkStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMBLINK_STUDIO_BUILD_TESTS "Build the Qt tests" ON)

# Qt 6.8 LTS is the release target (spec section 9); newer 6.x builds for development.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets LinguistTools)
qt_standard_project_setup()

# The Python that runs the core during development (spec section 11, question 4).
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if(EMBLINK_STUDIO_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

# Qt Advanced Docking System (spec section 11, question 1): LGPL-2.1-or-later, so it is built
# as a shared library and linked dynamically (spec section 9). Pinned by version and SHA-256.
include(FetchContent)
set(QT_VERSION_MAJOR 6)
set(BUILD_STATIC OFF CACHE BOOL "Build Qt Advanced Docking System as a static library" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build Qt Advanced Docking System's examples" FORCE)
FetchContent_Declare(qtads
    URL https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System/archive/refs/tags/v5.1.1.tar.gz
    URL_HASH SHA256=9ce947acbb1984946f9145c976e42e6984f99b98fb414dd7354a290af6d4ff04
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(qtads)
# Next to the application and the tests, so Windows finds the DLL without a PATH change.
set_target_properties(qtadvanceddocking-qt6 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app"
)

add_subdirectory(app)
