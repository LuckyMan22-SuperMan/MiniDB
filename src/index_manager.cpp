#include "minidb/index_manager.h"

#include <stdexcept>
#include <utility>

namespace minidb {

using namespace std;

IndexManager::IndexManager(BufferPoolManager& buffer_pool, Catalog& catalog)
    : buffer_pool_(buffer_pool), catalog_(catalog) {
    for (const string& table_name : catalog_.table_names()) {
        for (const IndexMetadata& metadata : catalog_.indexes_for_table(table_name)) {
            trees_.emplace(metadata.name,
                           make_unique<BPlusTree>(buffer_pool_, metadata.root_page_id));
        }
    }
}

bool IndexManager::create_index(const string& index_name, const string& table_name,
                                const string& column_name) {
    const optional<TableMetadata> table_metadata = catalog_.get_table(table_name);
    if (!table_metadata.has_value()) {
        throw invalid_argument("Unknown table: " + table_name);
    }
    if (catalog_.get_index(index_name).has_value()) {
        return false;
    }
    const size_t column_index = table_metadata->schema.column_index(column_name);
    if (table_metadata->schema.column(column_index).type() != Type::Int) {
        throw invalid_argument("Only INT columns can be indexed in Phase 15");
    }

    PageId root_page_id = kInvalidPageId;
    unique_ptr<BPlusTree> index = BPlusTree::create(buffer_pool_, root_page_id);
    if (index == nullptr) {
        throw runtime_error("Unable to allocate index root page");
    }
    const bool recorded = catalog_.create_index(
        IndexMetadata{index_name, table_name, column_name, root_page_id});
    if (!recorded) {
        return false;
    }
    trees_.emplace(index_name, std::move(index));
    return true;
}

void IndexManager::insert_tuple(const string& table_name, const Tuple& tuple, const RID& rid) {
    const optional<TableMetadata> table_metadata = catalog_.get_table(table_name);
    if (!table_metadata.has_value()) throw invalid_argument("Unknown table: " + table_name);
    for (const IndexMetadata& metadata : catalog_.indexes_for_table(table_name)) {
        if (!tree(metadata.name).insert(key_for(metadata, tuple), rid)) {
            throw runtime_error("Duplicate or full index entry: " + metadata.name);
        }
    }
}

void IndexManager::remove_tuple(const string& table_name, const Tuple& tuple, const RID& rid) {
    for (const IndexMetadata& metadata : catalog_.indexes_for_table(table_name)) {
        if (!tree(metadata.name).remove(key_for(metadata, tuple))) {
            throw runtime_error("Missing index entry: " + metadata.name);
        }
    }
    (void)rid;
}

void IndexManager::update_tuple(const string& table_name, const Tuple& old_tuple,
                                const Tuple& new_tuple, const RID& rid) {
    remove_tuple(table_name, old_tuple, rid);
    insert_tuple(table_name, new_tuple, rid);
}

optional<RID> IndexManager::search(const string& table_name, const string& column_name,
                                   int32_t key) const {
    for (const IndexMetadata& metadata : catalog_.indexes_for_table(table_name)) {
        if (metadata.column_name == column_name) {
            const auto found = trees_.find(metadata.name);
            if (found == trees_.end()) throw runtime_error("Index is not loaded: " + metadata.name);
            return found->second->search(key);
        }
    }
    throw invalid_argument("No index exists for column: " + column_name);
}

int32_t IndexManager::key_for(const IndexMetadata& metadata, const Tuple& tuple) const {
    const optional<TableMetadata> table_metadata = catalog_.get_table(metadata.table_name);
    if (!table_metadata.has_value()) throw invalid_argument("Unknown table: " + metadata.table_name);
    const size_t column_index = table_metadata->schema.column_index(metadata.column_name);
    return tuple.value_at(column_index).as_int();
}

BPlusTree& IndexManager::tree(const string& index_name) {
    const auto found = trees_.find(index_name);
    if (found == trees_.end()) throw runtime_error("Index is not loaded: " + index_name);
    return *found->second;
}

}  // namespace minidb
