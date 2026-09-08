#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace minidb {

class LRUReplacer {
public:
    explicit LRUReplacer(std::size_t capacity);

    LRUReplacer(const LRUReplacer&) = delete;
    LRUReplacer& operator=(const LRUReplacer&) = delete;

    void pin(std::size_t frame_id);
    void unpin(std::size_t frame_id);
    [[nodiscard]] std::optional<std::size_t> victim();
    [[nodiscard]] std::size_t size() const noexcept;

private:
    using FrameList = std::list<std::size_t>;
    using FrameIterator = FrameList::iterator;

    std::size_t capacity_;
    FrameList least_recently_used_;
    std::unordered_map<std::size_t, FrameIterator> frame_positions_;
};

}  // namespace minidb
