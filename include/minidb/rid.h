#pragma once

#include "minidb/page.h"

#include <cstdint>

namespace minidb {

struct RID {
    PageId page_id{kInvalidPageId};
    std::uint16_t slot_id{0};

    [[nodiscard]] bool operator==(const RID& other) const noexcept {
        return page_id == other.page_id && slot_id == other.slot_id;
    }
};

}  // namespace minidb
