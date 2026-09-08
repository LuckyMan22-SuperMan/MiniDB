cmake_minimum_required(VERSION 3.20)

project(minidb VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

enable_testing()

add_executable(minidb
    src/main.cpp
    src/disk_manager.cpp
    src/lru_replacer.cpp
    src/buffer_pool_manager.cpp
    src/value.cpp
    src/schema.cpp
    src/tuple.cpp
    src/table_page.cpp
    src/table_heap.cpp
)

target_compile_options(minidb PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(minidb_phase0_tests
    tests/phase0_smoke_test.cpp
)

target_compile_options(minidb_phase0_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_phase0_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME phase0_smoke COMMAND minidb_phase0_tests)

add_executable(minidb_disk_manager_tests
    src/disk_manager.cpp
    tests/disk_manager_test.cpp
)

target_compile_options(minidb_disk_manager_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_disk_manager_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME disk_manager COMMAND minidb_disk_manager_tests)

add_executable(minidb_lru_tests
    src/lru_replacer.cpp
    tests/lru_replacer_test.cpp
)

target_compile_options(minidb_lru_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_lru_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME lru_replacer COMMAND minidb_lru_tests)

add_executable(minidb_buffer_pool_tests
    src/disk_manager.cpp
    src/lru_replacer.cpp
    src/buffer_pool_manager.cpp
    tests/buffer_pool_manager_test.cpp
)

target_compile_options(minidb_buffer_pool_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_buffer_pool_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME buffer_pool_manager COMMAND minidb_buffer_pool_tests)

add_executable(minidb_tuple_tests
    src/value.cpp
    src/schema.cpp
    src/tuple.cpp
    tests/tuple_test.cpp
)

target_compile_options(minidb_tuple_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_tuple_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME tuple_serialization COMMAND minidb_tuple_tests)

add_executable(minidb_table_heap_tests
    src/disk_manager.cpp
    src/lru_replacer.cpp
    src/buffer_pool_manager.cpp
    src/value.cpp
    src/schema.cpp
    src/tuple.cpp
    src/table_page.cpp
    src/table_heap.cpp
    tests/table_heap_test.cpp
)

target_compile_options(minidb_table_heap_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_include_directories(minidb_table_heap_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_test(NAME table_heap COMMAND minidb_table_heap_tests)
