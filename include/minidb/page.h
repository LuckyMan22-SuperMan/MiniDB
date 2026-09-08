#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace minidb {

using PageId = std::int32_t;

inline constexpr std::size_t kPageSize = 4096;
inline constexpr PageId kInvalidPageId = -1;

class Page {
public:
    Page() = default;

    [[nodiscard]] PageId id() const noexcept { return id_; }
    void set_id(PageId id) noexcept { id_ = id; }

    [[nodiscard]] std::byte* data() noexcept { return data_.data(); }
    [[nodiscard]] const std::byte* data() const noexcept { return data_.data(); }

    void reset() noexcept { data_.fill(std::byte{0}); }

private:
    PageId id_{kInvalidPageId};
    std::array<std::byte, kPageSize> data_{};
};

}  // namespace minidb
