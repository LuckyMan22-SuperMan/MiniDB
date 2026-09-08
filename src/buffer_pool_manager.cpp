#include "minidb/buffer_pool_manager.h"

#include <stdexcept>

namespace minidb {

using namespace std;

BufferPoolManager::BufferPoolManager(DiskManager& disk_manager, size_t pool_size)
    : disk_manager_(disk_manager), frames_(pool_size), replacer_(pool_size) {
    if (pool_size == 0) {
        throw invalid_argument("Buffer pool size must be greater than zero");
    }
}

Page* BufferPoolManager::new_page(PageId& page_id) {
    const auto frame_id = find_frame_for_use();
    if (!frame_id.has_value()) {
        page_id = kInvalidPageId;
        return nullptr;
    }

    page_id = disk_manager_.allocate_page();
    Frame& frame = frames_[*frame_id];
    frame.page.reset();
    frame.page.set_id(page_id);
    frame.pin_count = 1;
    frame.is_dirty = false;
    page_table_[page_id] = *frame_id;
    replacer_.pin(*frame_id);
    return &frame.page;
}

Page* BufferPoolManager::fetch_page(PageId page_id) {
    const auto existing_page = page_table_.find(page_id);
    if (existing_page != page_table_.end()) {
        Frame& frame = frames_[existing_page->second];
        ++frame.pin_count;
        replacer_.pin(existing_page->second);
        return &frame.page;
    }

    const auto frame_id = find_frame_for_use();
    if (!frame_id.has_value()) {
        return nullptr;
    }

    Frame& frame = frames_[*frame_id];
    disk_manager_.read_page(page_id, frame.page);
    frame.pin_count = 1;
    frame.is_dirty = false;
    page_table_[page_id] = *frame_id;
    replacer_.pin(*frame_id);
    return &frame.page;
}

bool BufferPoolManager::unpin_page(PageId page_id, bool is_dirty) {
    const auto frame_id = page_table_.find(page_id);
    if (frame_id == page_table_.end()) {
        return false;
    }

    Frame& frame = frames_[frame_id->second];
    if (frame.pin_count == 0) {
        return false;
    }

    --frame.pin_count;
    frame.is_dirty = frame.is_dirty || is_dirty;
    if (frame.pin_count == 0) {
        replacer_.unpin(frame_id->second);
    }
    return true;
}
    using namespace std;

bool BufferPoolManager::flush_page(PageId page_id) {
    const auto frame_id = page_table_.find(page_id);
    if (frame_id == page_table_.end()) {
            throw invalid_argument("Buffer pool size must be greater than zero");
    }

    flush_frame(frame_id->second);
    return true;
}

void BufferPoolManager::flush_all_pages() {
    for (size_t frame_id = 0; frame_id < frames_.size(); ++frame_id) {
        if (frames_[frame_id].page.id() != kInvalidPageId) {
            flush_frame(frame_id);
        }
    }
}

bool BufferPoolManager::delete_page(PageId page_id) {
    const auto frame_id = page_table_.find(page_id);
    if (frame_id != page_table_.end()) {
        Frame& frame = frames_[frame_id->second];
        if (frame.pin_count != 0) {
            return false;
        }

        replacer_.pin(frame_id->second);
        frame.page.reset();
        frame.page.set_id(kInvalidPageId);
        frame.is_dirty = false;
        page_table_.erase(frame_id);
    }

    disk_manager_.deallocate_page(page_id);
    return true;
}

size_t BufferPoolManager::pool_size() const noexcept {
    return frames_.size();
}

optional<size_t> BufferPoolManager::find_frame_for_use() {
    for (size_t frame_id = 0; frame_id < frames_.size(); ++frame_id) {
        if (frames_[frame_id].page.id() == kInvalidPageId) {
            return frame_id;
        }
    }

    const auto victim = replacer_.victim();
    if (!victim.has_value()) {
        return std::nullopt;
    }

    Frame& frame = frames_[*victim];
    const PageId old_page_id = frame.page.id();
    if (frame.is_dirty) {
        flush_frame(*victim);
    }
    page_table_.erase(old_page_id);
    frame.page.set_id(kInvalidPageId);
    frame.is_dirty = false;
    frame.pin_count = 0;
    return victim;
}

size_t BufferPoolManager::frame_for(PageId page_id) const {
    const auto frame_id = page_table_.find(page_id);
    if (frame_id == page_table_.end()) {
        throw out_of_range("Page is not in the buffer pool");
    }
    return frame_id->second;
}

void BufferPoolManager::flush_frame(size_t frame_id) {
    Frame& frame = frames_[frame_id];
    if (frame.page.id() == kInvalidPageId) {
        return;
    }

    if (frame.is_dirty) {
        disk_manager_.write_page(frame.page.id(), frame.page);
        frame.is_dirty = false;
    }
}

}  // namespace minidb
