#include "minidb/execution.h"
#include "minidb/parser.h"

#include <cassert>
#include <filesystem>
#include <string>

using namespace std;

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase18_data.db";
    const auto catalog_path = filesystem::temp_directory_path() / "minidb_phase18_catalog.db";
    filesystem::remove(database_path);
    filesystem::remove(catalog_path);

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 16);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);

        const auto created = engine.execute(minidb::Parser::parse_sql(
            "CREATE TABLE inventory (id INT, name VARCHAR, active BOOLEAN);"));
        (void)created;
        const auto index = engine.execute(minidb::Parser::parse_sql(
            "CREATE INDEX idx_inventory_id ON inventory(id);"));
        (void)index;
        const auto inserted = engine.execute(minidb::Parser::parse_sql(
            "INSERT INTO inventory VALUES (1, 'seed', TRUE);"));
        (void)inserted;
        const auto selected = engine.execute(minidb::Parser::parse_sql(
            "SELECT name FROM inventory WHERE id = 1;"));
        assert(selected.rows.size() == 1);
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 16);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);

        const auto restored = engine.execute(minidb::Parser::parse_sql(
            "SELECT * FROM inventory WHERE id = 1;"));
        assert(restored.rows.size() == 1);
        assert(restored.rows[0][1].as_varchar() == "seed");

        const auto updated = engine.execute(minidb::Parser::parse_sql(
            "UPDATE inventory SET name = 'done' WHERE id = 1;"));
        assert(updated.affected_rows == 1);
        const auto deleted = engine.execute(minidb::Parser::parse_sql(
            "DELETE FROM inventory WHERE id = 1;"));
        assert(deleted.affected_rows == 1);
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 16);
        minidb::Catalog catalog(catalog_path);
        minidb::ExecutionEngine engine(buffer_pool, catalog);
        const auto final_state = engine.execute(minidb::Parser::parse_sql(
            "SELECT * FROM inventory;"));
        assert(final_state.rows.empty());
        assert(catalog.get_index("idx_inventory_id").has_value());
    }

    filesystem::remove(database_path);
    filesystem::remove(catalog_path);
    return 0;
}
