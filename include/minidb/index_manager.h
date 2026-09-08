#pragma once

#include "minidb/b_plus_tree.h"
#include "minidb/catalog.h"
#include "minidb/tuple.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace minidb {

class IndexManager {
public:
    IndexManager(BufferPoolManager& buffer_pool, Catalog& catalog);

    [[nodiscard]] bool create_index(const std::string& index_name,
                                    const std::string& table_name,
                                    const std::string& column_name);
    void insert_tuple(const std::string& table_name, const Tuple& tuple, const RID& rid);
    void remove_tuple(const std::string& table_name, const Tuple& tuple, const RID& rid);
    void update_tuple(const std::string& table_name, const Tuple& old_tuple,
                      const Tuple& new_tuple, const RID& rid);

private:
    [[nodiscard]] std::int32_t key_for(const IndexMetadata& metadata,
                                       const Tuple& tuple) const;
    [[nodiscard]] BPlusTree& tree(const std::string& index_name);

    BufferPoolManager& buffer_pool_;
    Catalog& catalog_;
    std::unordered_map<std::string, std::unique_ptr<BPlusTree>> trees_;
};

}  // namespace minidb
