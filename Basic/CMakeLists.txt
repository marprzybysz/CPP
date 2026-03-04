cmake_minimum_required(VERSION 3.20)
project(my_project VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(my_project
    src/main.cpp
    src/app.cpp
)

target_include_directories(my_project PRIVATE src)

# Przydatne ostrzeżenia (dla GCC/Clang)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(my_project PRIVATE -Wall -Wextra -Wpedantic)
endif()