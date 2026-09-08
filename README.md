# MiniDB

MiniDB is an educational relational database management system implemented from scratch in modern C++20. The project is being built incrementally to make storage, query processing, and indexing concepts concrete.

## Current Status

Phase 3 implements the fixed-size `Page` abstraction, file-backed `DiskManager`, bounded `BufferPoolManager`, `LRUReplacer`, and the first record layer: `Value`, `Column`, `Schema`, `Tuple`, and `RID`. Slotted table pages and higher-level storage layers are not implemented yet.

Planned components include:

- Page-oriented disk and buffer-pool storage
- Tuple, schema, table heap, and catalog layers
- A small SQL lexer, parser, AST, planner, and executor
- Sequential scans and basic data modification statements
- A persistent B+ Tree index and index-aware scans
- An interactive SQL command-line interface

## Project Layout

```text
.
├── CMakeLists.txt
├── include/       Public headers for future components
├── src/           Application and library implementation
├── tests/         Focused subsystem tests
└── build/         Local build output (ignored by Git)
```

## Requirements

- macOS on Apple Silicon or another platform with a C++20 compiler
- Clang/LLVM
- CMake 3.20 or newer
- Ninja
- Git
- LLDB for debugging

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

The Phase 1 disk-manager test checks page allocation, persistence across a close and reopen, page counting, and invalid page rejection. Phase 2 tests check LRU ordering, pinned-page protection, dirty-page write-back, eviction, and deletion. Phase 3 tests check schema lookup, typed values, tuple serialization round trips, RIDs, and malformed-data rejection.

## Run

```sh
./build/minidb
```

Expected output:

```text
MiniDB v0.1 - Phase 0 skeleton
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

## Debug in VS Code

Open the project folder in VS Code. Configure the project with the build command above, then launch `build/minidb` under the C++ extension using LLDB. The executable currently has no database behavior; it only confirms that the C++20 application target is wired correctly.

## Development Approach

The implementation will proceed phase by phase. Each phase will keep the project buildable and will add focused tests before the next storage or query layer is introduced.
