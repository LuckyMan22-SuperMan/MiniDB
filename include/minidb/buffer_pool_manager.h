#pragma once

#include "minidb/disk_manager.h"
#include "minidb/lru_replacer.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace minidb {

class BufferPoolManager {
public:
    BufferPoolManager(DiskManager& disk_manager, std::size_t pool_size);

    BufferPoolManager(const BufferPoolManager&) = delete;
    BufferPoolManager& operator=(const BufferPoolManager&) = delete;

    [[nodiscard]] Page* new_page(PageId& page_id);
    [[nodiscard]] Page* fetch_page(PageId page_id);
    [[nodiscard]] bool unpin_page(PageId page_id, bool is_dirty);
    [[nodiscard]] bool flush_page(PageId page_id);
    void flush_all_pages();
    [[nodiscard]] bool delete_page(PageId page_id);

    [[nodiscard]] std::size_t pool_size() const noexcept;

private:
    struct Frame {
        Page page;
        std::size_t pin_count{0};
        bool is_dirty{false};
    };

    [[nodiscard]] std::optional<std::size_t> find_frame_for_use();
    [[nodiscard]] std::size_t frame_for(PageId page_id) const;
    void flush_frame(std::size_t frame_id);

    DiskManager& disk_manager_;
    std::vector<Frame> frames_;
    std::unordered_map<PageId, std::size_t> page_table_;
    LRUReplacer replacer_;
};

}  // namespace minidb
