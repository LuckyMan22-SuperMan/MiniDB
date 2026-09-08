# MiniDB

MiniDB is an educational relational database management system implemented from scratch in modern C++20. The project is being built incrementally to make storage, query processing, and indexing concepts concrete.

## Current Status

Phase 16 implements the fixed-size `Page` abstraction, file-backed `DiskManager`, bounded `BufferPoolManager`, `LRUReplacer`, the record layer, slotted `TablePage`, `TableHeap`, a persistent catalog, the SQL lexer, a typed recursive-descent parser with AST nodes, typed plan nodes including `IndexScan`, execution for `CREATE TABLE`, `INSERT`, sequential or indexed `SELECT`, robust `WHERE` filtering, `UPDATE`, `DELETE`, and a persistent B+ Tree with leaf splitting, deletion, empty-leaf cleanup, root collapse, and Index Manager synchronization. The interactive CLI and general internal-node rebalancing are not implemented yet.

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

The Phase 1 disk-manager test checks page allocation, persistence across a close and reopen, page counting, and invalid page rejection. Phase 2 tests check LRU ordering, pinned-page protection, dirty-page write-back, eviction, and deletion. Phase 3 tests check schema lookup, typed values, tuple serialization round trips, RIDs, and malformed-data rejection. Phase 4 tests check slotted-page insertion, tuple deletion, multi-page heap growth, RID lookup, and flushing. Phase 5 tests check catalog creation, duplicate-table rejection, metadata updates, and restart persistence. Phase 6 tests check SQL keyword recognition, literals, comments, operators, escaped strings, and lexical errors. Phase 7 tests check AST construction for all supported statements, typed literals, logical WHERE expressions, and syntax errors. Phase 8 tests check plan selection for scans, filters, DML, and DDL. Phase 9 tests check execution, sequential scanning, filtering, and restart reads. Phase 10 tests check parenthesized expressions, boolean predicates, comparison operators, and logical precedence. Phase 11 tests check stable-RID updates, deletion, affected-row counts, persistence, and oversized-update errors. Phase 12 tests check sorted index insertion, duplicate rejection, RID search, and index restart persistence. Phase 13 tests force leaf splitting, root routing, multi-page search, and restart reads. Phase 14 tests cover deletion, empty-leaf cleanup, root collapse, and reinsertion. Phase 15 tests check persistent index metadata, SQL `CREATE INDEX`, indexed mutation synchronization, restart loading, and duplicate-key rollback.

Indexes currently support `INT` columns only. Only simple equality predicates on indexed columns use `IndexScan`; range and compound predicates continue to use sequential scans.

Updates currently require the replacement tuple to fit in the existing slot; larger replacements fail explicitly rather than changing the row's RID. Page compaction and relocation can be added with later storage hardening.

`TableHeap` still keeps its active page list in memory; the catalog now persists the authoritative table metadata and page list, but automatic reconstruction of `TableHeap` instances from catalog entries will be integrated with the execution layer.

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
