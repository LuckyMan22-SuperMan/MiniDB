#pragma once

#include "minidb/page.h"
#include "minidb/schema.h"
#include "minidb/tuple.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace minidb {

class TablePage {
public:
    explicit TablePage(Page& page);

    void initialize();

    [[nodiscard]] bool insert_tuple(const Tuple& tuple, const Schema& schema,
                                    std::uint16_t& slot_id);
    [[nodiscard]] bool update_tuple(const Tuple& tuple, const Schema& schema,
                                    std::uint16_t slot_id);
    [[nodiscard]] std::optional<Tuple> get_tuple(const Schema& schema,
                                                 std::uint16_t slot_id) const;
    [[nodiscard]] bool delete_tuple(std::uint16_t slot_id);

    [[nodiscard]] std::uint16_t slot_count() const;
    [[nodiscard]] std::size_t free_space() const;

private:
    struct Slot {
        std::uint16_t offset;
        std::uint16_t length;
    };

    [[nodiscard]] Slot slot_at(std::uint16_t slot_id) const;
    void set_slot(std::uint16_t slot_id, Slot slot);
    [[nodiscard]] std::uint16_t free_space_end() const;
    void set_free_space_end(std::uint16_t offset);
    [[nodiscard]] bool has_valid_header() const;

    Page& page_;
};

}  // namespace minidb
