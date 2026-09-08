#pragma once

#include "minidb/buffer_pool_manager.h"
#include "minidb/rid.h"
#include "minidb/schema.h"
#include "minidb/tuple.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace minidb {

class TableHeap {
public:
    TableHeap(BufferPoolManager& buffer_pool, Schema schema);

    TableHeap(const TableHeap&) = delete;
    TableHeap& operator=(const TableHeap&) = delete;

    [[nodiscard]] std::optional<RID> insert_tuple(const Tuple& tuple);
    [[nodiscard]] std::optional<Tuple> get_tuple(const RID& rid);
    [[nodiscard]] bool delete_tuple(const RID& rid);
    void flush();

    [[nodiscard]] const Schema& schema() const noexcept;
    [[nodiscard]] const std::vector<PageId>& page_ids() const noexcept;

private:
    BufferPoolManager& buffer_pool_;
    Schema schema_;
    std::vector<PageId> page_ids_;
};

}  // namespace minidb
