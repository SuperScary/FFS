cmake_minimum_required(VERSION 3.20)
project(FFS VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure version header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/version.h"
    @ONLY
)

add_executable(FFS
        src/main.cpp
        src/util.cpp
        src/compiler.cpp
        src/vm.cpp

        # Headers
        include/compiler.h
        include/program.h
        include/util.h
        include/vm.h
)

target_include_directories(FFS
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/include
)
