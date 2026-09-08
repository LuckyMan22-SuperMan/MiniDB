#include "minidb/execution.h"
#include "minidb/parser.h"

#include <cassert>
#include <filesystem>
#include <string>

using namespace std;

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase9_data.db";
    const auto catalog_path = filesystem::temp_directory_path() / "minidb_phase9_catalog.db";
    filesystem::remove(database_path);
    filesystem::remove(catalog_path);

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 3);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);

        const auto create = engine.execute(minidb::Parser::parse_sql(
            "CREATE TABLE students (id INT, name VARCHAR, active BOOLEAN);"));
        assert(create.affected_rows == 0);

        assert(engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (1, 'Lakshya', TRUE);"))
                   .affected_rows == 1);
        assert(engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (2, 'Ada', FALSE);"))
                   .affected_rows == 1);
        assert(engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO students VALUES (3, 'Grace', TRUE);"))
                   .affected_rows == 1);

        const auto all_rows = engine.execute(minidb::Parser::parse_sql("SELECT * FROM students;"));
        assert(all_rows.columns.size() == 3);
        assert(all_rows.rows.size() == 3);

        const auto filtered = engine.execute(minidb::Parser::parse_sql(
            "SELECT name FROM students WHERE id = 2;"));
        assert(filtered.rows.size() == 1);
        assert(filtered.rows[0][0].as_varchar() == "Ada");

        const auto grouped = engine.execute(minidb::Parser::parse_sql(
            "SELECT id FROM students WHERE (id >= 2 AND id < 3) OR active = TRUE;"));
        assert(grouped.rows.size() == 3);
        assert(grouped.rows[0][0].as_int() == 1);
        assert(grouped.rows[1][0].as_int() == 2);
        assert(grouped.rows[2][0].as_int() == 3);

        const auto boolean_predicate = engine.execute(minidb::Parser::parse_sql(
            "SELECT name FROM students WHERE active;"));
        assert(boolean_predicate.rows.size() == 2);

        const auto greater_than = engine.execute(minidb::Parser::parse_sql(
            "SELECT id FROM students WHERE id > 1;"));
        assert(greater_than.rows.size() == 2);
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 3);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);
        const auto restored = engine.execute(minidb::Parser::parse_sql(
            "SELECT name FROM students WHERE active = TRUE;"));
        assert(restored.rows.size() == 2);
        assert(restored.rows[0][0].as_varchar() == "Lakshya");
    }

    filesystem::remove(database_path);
    filesystem::remove(catalog_path);
    return 0;
}
