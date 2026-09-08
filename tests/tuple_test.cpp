#include "minidb/rid.h"
#include "minidb/tuple.h"

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

int main() {
    const minidb::Schema schema({
        minidb::Column("id", minidb::Type::Int),
        minidb::Column("name", minidb::Type::Varchar),
        minidb::Column("active", minidb::Type::Boolean),
    });

    assert(schema.column_count() == 3);
    assert(schema.column_index("name") == 1);
    assert(schema.column(0).name() == "id");

    const minidb::Tuple original({
        minidb::Value(static_cast<int32_t>(42)),
        minidb::Value(string("Lakshya")),
        minidb::Value(true),
    });
    const vector<byte> serialized = original.serialize(schema);
    const minidb::Tuple restored = minidb::Tuple::deserialize(schema, serialized);

    assert(restored.size() == 3);
    assert(restored.value_at(0).as_int() == 42);
    assert(restored.value_at(1).as_varchar() == "Lakshya");
    assert(restored.value_at(2).as_boolean());
    assert(restored.value_at(0) == original.value_at(0));

    const minidb::RID first_rid{7, 3};
    const minidb::RID second_rid{7, 3};
    assert(first_rid == second_rid);

    bool rejected_wrong_count = false;
    try {
        const auto ignored = minidb::Tuple({minidb::Value(static_cast<int32_t>(1))}).serialize(schema);
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_wrong_count = true;
    }
    assert(rejected_wrong_count);

    bool rejected_truncated_data = false;
    try {
        vector<byte> truncated = serialized;
        truncated.pop_back();
        const auto ignored = minidb::Tuple::deserialize(schema, truncated);
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_truncated_data = true;
    }
    assert(rejected_truncated_data);

    bool rejected_trailing_data = false;
    try {
        vector<byte> trailing = serialized;
        trailing.push_back(byte{0});
        const auto ignored = minidb::Tuple::deserialize(schema, trailing);
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_trailing_data = true;
    }
    assert(rejected_trailing_data);

    bool rejected_unknown_column = false;
    try {
        const auto ignored = schema.column_index("missing");
        (void)ignored;
    } catch (const out_of_range&) {
        rejected_unknown_column = true;
    }
    assert(rejected_unknown_column);
    return 0;
}
