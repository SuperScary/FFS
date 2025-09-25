cmake_minimum_required(VERSION 4.0)
project(FFS)

set(CMAKE_CXX_STANDARD 20)

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
)
