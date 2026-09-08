#include "minidb/buffer_pool_manager.h"
#include "minidb/catalog.h"
#include "minidb/execution.h"
#include "minidb/parser.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <stdexcept>

using namespace std;

int main() {
    bool rejected_bad_sql = false;
    try {
        const auto ignored = minidb::Parser::parse_sql("SELECT FROM;");
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_bad_sql = true;
    }
    assert(rejected_bad_sql);

    bool rejected_zero_pool = false;
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase19_data.db";
    const auto catalog_path = filesystem::temp_directory_path() / "minidb_phase19_catalog.db";
    filesystem::remove(database_path);
    filesystem::remove(catalog_path);
    try {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 0);
        (void)buffer_pool;
    } catch (const invalid_argument&) {
        rejected_zero_pool = true;
    }
    assert(rejected_zero_pool);

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 4);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);
        const auto created = engine.execute(minidb::Parser::parse_sql(
            "CREATE TABLE students (id INT, name VARCHAR);"));
        (void)created;

        bool rejected_duplicate_table = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "CREATE TABLE students (id INT);"));
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_duplicate_table = true;
        }
        assert(rejected_duplicate_table);

        bool rejected_wrong_arity = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "INSERT INTO students VALUES (1);"));
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_wrong_arity = true;
        }
        assert(rejected_wrong_arity);

        bool rejected_wrong_type = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "INSERT INTO students VALUES ('wrong', 'name');"));
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_wrong_type = true;
        }
        assert(rejected_wrong_type);

        bool rejected_unknown_table = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "SELECT * FROM missing;"));
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_unknown_table = true;
        }
        assert(rejected_unknown_table);

        bool rejected_unknown_column = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "SELECT missing FROM students;"));
            (void)ignored;
        } catch (const out_of_range&) {
            rejected_unknown_column = true;
        }
        assert(rejected_unknown_column);

        bool rejected_varchar_index = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "CREATE INDEX idx_name ON students(name);"));
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_varchar_index = true;
        }
        assert(rejected_varchar_index);
    }

    const auto corrupt_catalog = filesystem::temp_directory_path() / "minidb_phase19_corrupt.db";
    {
        ofstream output(corrupt_catalog, ios::binary);
        output << "corrupt catalog";
    }
    bool rejected_corrupt_catalog = false;
    try {
        const minidb::Catalog ignored(corrupt_catalog);
        (void)ignored;
    } catch (const runtime_error&) {
        rejected_corrupt_catalog = true;
    }
    assert(rejected_corrupt_catalog);

    filesystem::remove(database_path);
    filesystem::remove(catalog_path);
    filesystem::remove(corrupt_catalog);
    return 0;
}
