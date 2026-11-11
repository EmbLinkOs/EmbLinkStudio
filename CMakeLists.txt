# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 Junior Deogracias
cmake_minimum_required(VERSION 3.21)

project(EmbLinkStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(EMBLINK_STUDIO_BUILD_TESTS "Build the Qt tests" ON)

# Qt 6.8 LTS is the release target (spec section 9); newer 6.x builds for development.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()

# The Python that runs the core during development (spec section 11, question 4).
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if(EMBLINK_STUDIO_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

add_subdirectory(app)
