#include "minidb/execution.h"
#include "minidb/index_manager.h"
#include "minidb/parser.h"

#include <cassert>
#include <filesystem>

using namespace std;

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase15_data.db";
    const auto catalog_path = filesystem::temp_directory_path() / "minidb_phase15_catalog.db";
    filesystem::remove(database_path);
    filesystem::remove(catalog_path);

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 8);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);
        const auto created = engine.execute(minidb::Parser::parse_sql(
            "CREATE TABLE students (id INT, name VARCHAR);"));
        (void)created;
        const auto inserted_first = engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (1, 'A');"));
        (void)inserted_first;
        const auto inserted_second = engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (2, 'B');"));
        (void)inserted_second;
        const auto created_index = engine.execute(minidb::Parser::parse_sql(
            "CREATE INDEX idx_students_id ON students(id);"));
        (void)created_index;
        const auto inserted_third = engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (3, 'C');"));
        assert(inserted_third.affected_rows == 1);
        const auto updated = engine.execute(minidb::Parser::parse_sql(
            "UPDATE students SET name = 'D' WHERE id = 3;"));
        assert(updated.affected_rows == 1);
        const auto deleted = engine.execute(minidb::Parser::parse_sql(
            "DELETE FROM students WHERE id = 2;"));
        assert(deleted.affected_rows == 1);
        assert(catalog.get_index("idx_students_id")->root_page_id >= 0);
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 8);
        minidb::Catalog catalog(catalog_path);
        assert(catalog.get_index("idx_students_id").has_value());
        const auto metadata = catalog.get_table("students");
        assert(metadata.has_value());
        minidb::ExecutionEngine engine(buffer_pool, catalog);
        bool rejected_duplicate_key = false;
        try {
            const auto ignored = engine.execute(minidb::Parser::parse_sql(
                "INSERT INTO students VALUES (1, 'X');"));
            (void)ignored;
        } catch (const runtime_error&) {
            rejected_duplicate_key = true;
        }
        assert(rejected_duplicate_key);
        const auto remaining = engine.execute(minidb::Parser::parse_sql(
            "SELECT id FROM students;"));
        assert(remaining.rows.size() == 2);
    }

    filesystem::remove(database_path);
    filesystem::remove(catalog_path);
    return 0;
}
