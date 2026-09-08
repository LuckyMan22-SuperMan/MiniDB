#include "minidb/table_heap.h"
#include "minidb/table_page.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

using namespace std;

int main() {
    const minidb::Schema schema({
        minidb::Column("id", minidb::Type::Int),
        minidb::Column("name", minidb::Type::Varchar),
    });

    minidb::Page raw_page;
    raw_page.set_id(0);
    minidb::TablePage table_page(raw_page);
    table_page.initialize();
    const minidb::Tuple first_tuple({
        minidb::Value(static_cast<int32_t>(1)),
        minidb::Value(string("first")),
    });
    uint16_t first_slot = 0;
    assert(table_page.insert_tuple(first_tuple, schema, first_slot));
    assert(first_slot == 0);
    assert(table_page.slot_count() == 1);
    assert(table_page.get_tuple(schema, first_slot)->value_at(1).as_varchar() == "first");
    assert(table_page.delete_tuple(first_slot));
    assert(!table_page.get_tuple(schema, first_slot).has_value());
    assert(!table_page.delete_tuple(first_slot));

    const auto database_path = filesystem::temp_directory_path() / "minidb_phase4_test.db";
    filesystem::remove(database_path);
    minidb::DiskManager disk_manager(database_path);
    minidb::BufferPoolManager buffer_pool(disk_manager, 3);
    minidb::TableHeap table_heap(buffer_pool, schema);

    vector<minidb::RID> records;
    for (int32_t id = 0; id < 300; ++id) {
        const minidb::Tuple tuple({
            minidb::Value(id),
            minidb::Value(string("student-") + to_string(id)),
        });
        const auto rid = table_heap.insert_tuple(tuple);
        assert(rid.has_value());
        records.push_back(*rid);
    }

    assert(table_heap.page_ids().size() > 1);
    table_heap.flush();

    for (size_t index = 0; index < records.size(); index += 11) {
        const auto tuple = table_heap.get_tuple(records[index]);
        assert(tuple.has_value());
        assert(tuple->value_at(0).as_int() == static_cast<int32_t>(index));
        assert(tuple->value_at(1).as_varchar() == "student-" + to_string(index));
    }

    assert(table_heap.delete_tuple(records[10]));
    assert(!table_heap.get_tuple(records[10]).has_value());

    filesystem::remove(database_path);
    return 0;
}
