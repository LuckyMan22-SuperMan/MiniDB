#pragma once

#include "minidb/buffer_pool_manager.h"
#include "minidb/rid.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace minidb {

class BPlusTree {
public:
    BPlusTree(BufferPoolManager& buffer_pool, PageId root_page_id);

    [[nodiscard]] static std::unique_ptr<BPlusTree> create(BufferPoolManager& buffer_pool,
                                                            PageId& root_page_id);
    [[nodiscard]] bool insert(std::int32_t key, RID rid);
    [[nodiscard]] std::optional<RID> search(std::int32_t key) const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] PageId root_page_id() const noexcept;

private:
    BufferPoolManager& buffer_pool_;
    PageId root_page_id_;
};

}  // namespace minidb
