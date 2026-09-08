#include "minidb/lru_replacer.h"

#include <stdexcept>

namespace minidb {

using namespace std;

LRUReplacer::LRUReplacer(size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
        throw invalid_argument("LRU replacer capacity must be greater than zero");
    }
}

void LRUReplacer::pin(size_t frame_id) {
    const auto position = frame_positions_.find(frame_id);
    if (position == frame_positions_.end()) {
        return;
    }

    least_recently_used_.erase(position->second);
    frame_positions_.erase(position);
}

void LRUReplacer::unpin(size_t frame_id) {
    if (frame_positions_.contains(frame_id)) {
        return;
    }

    if (least_recently_used_.size() == capacity_) {
        const size_t evicted_frame = least_recently_used_.back();
        least_recently_used_.pop_back();
        frame_positions_.erase(evicted_frame);
    }

    least_recently_used_.push_front(frame_id);
    frame_positions_[frame_id] = least_recently_used_.begin();
}

optional<size_t> LRUReplacer::victim() {
    if (least_recently_used_.empty()) {
        return nullopt;
    }

    const size_t frame_id = least_recently_used_.back();
    least_recently_used_.pop_back();
    frame_positions_.erase(frame_id);
    return frame_id;
}

size_t LRUReplacer::size() const noexcept {
    return least_recently_used_.size();
}

}  // namespace minidb
