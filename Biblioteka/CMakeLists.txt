cmake_minimum_required(VERSION 3.20)
project(cpp_biblioteka VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cpp_biblioteka
    src/main.cpp
    src/db.cpp
    src/library.cpp
    src/common/system_id.cpp
    src/books/book_service.cpp
    src/books/sqlite_book_repository.cpp
    src/copies/copy.cpp
    src/copies/copy_service.cpp
    src/copies/sqlite_copy_repository.cpp
    src/locations/location.cpp
    src/locations/location_service.cpp
    src/locations/sqlite_location_repository.cpp
    src/errors/error_mapper.cpp
    src/errors/error_logger.cpp
)

target_include_directories(cpp_biblioteka PRIVATE include)

# SQLite3
find_package(SQLite3 REQUIRED)
target_link_libraries(cpp_biblioteka PRIVATE SQLite::SQLite3)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(cpp_biblioteka PRIVATE -Wall -Wextra -Wpedantic)
endif()
