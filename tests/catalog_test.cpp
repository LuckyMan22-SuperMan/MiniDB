#include "minidb/catalog.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

int main() {
    const auto catalog_path = filesystem::temp_directory_path() / "minidb_phase5_catalog.db";
    filesystem::remove(catalog_path);

    const minidb::Schema schema({
        minidb::Column("id", minidb::Type::Int),
        minidb::Column("name", minidb::Type::Varchar),
        minidb::Column("active", minidb::Type::Boolean),
    });

    uint32_t table_id = 0;
    {
        minidb::Catalog catalog(catalog_path);
        table_id = catalog.create_table("students", schema, {3, 8});
        assert(table_id == 0);
        assert(catalog.table_names().size() == 1);
        assert(catalog.get_table("students")->page_ids == vector<minidb::PageId>({3, 8}));
        assert(catalog.update_table_pages("students", {3, 8, 12}));
        assert(!catalog.update_table_pages("missing", {1}));

        bool rejected_duplicate = false;
        try {
            const auto ignored = catalog.create_table("students", schema);
            (void)ignored;
        } catch (const invalid_argument&) {
            rejected_duplicate = true;
        }
        assert(rejected_duplicate);
    }

    {
        minidb::Catalog reopened_catalog(catalog_path);
        const auto table = reopened_catalog.get_table("students");
        assert(table.has_value());
        assert(table->table_id == table_id);
        assert(table->schema.column_count() == 3);
        assert(table->schema.column(0).name() == "id");
        assert(table->schema.column(1).type() == minidb::Type::Varchar);
        assert(table->page_ids == vector<minidb::PageId>({3, 8, 12}));

        const uint32_t second_table_id = reopened_catalog.create_table(
            "courses", minidb::Schema({minidb::Column("code", minidb::Type::Varchar)}));
        assert(second_table_id == 1);
    }

    filesystem::remove(catalog_path);
    return 0;
}
