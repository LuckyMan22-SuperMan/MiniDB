#include "minidb/table_page.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace minidb {

namespace {

constexpr std::size_t kSlotCountOffset = 0;
constexpr std::size_t kFreeSpaceEndOffset = sizeof(std::uint16_t);
constexpr std::size_t kHeaderSize = sizeof(std::uint16_t) * 2;
constexpr std::size_t kSlotSize = sizeof(std::uint16_t) * 2;

void read_uint16(const Page& page, std::size_t offset, std::uint16_t& value) {
    std::memcpy(&value, page.data() + offset, sizeof(value));
}

void write_uint16(Page& page, std::size_t offset, std::uint16_t value) {
    std::memcpy(page.data() + offset, &value, sizeof(value));
}

}  // namespace

TablePage::TablePage(Page& page) : page_(page) {}

void TablePage::initialize() {
    page_.reset();
    set_free_space_end(static_cast<std::uint16_t>(kPageSize));
}

bool TablePage::insert_tuple(const Tuple& tuple, const Schema& schema, std::uint16_t& slot_id) {
    if (!has_valid_header()) {
        throw std::logic_error("Table page is not initialized");
    }

    const std::vector<std::byte> bytes = tuple.serialize(schema);
    if (bytes.size() > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }

    const std::uint16_t current_slot_count = slot_count();
    const std::size_t directory_end = kHeaderSize + (current_slot_count + 1) * kSlotSize;
    if (bytes.size() + directory_end > free_space_end()) {
        return false;
    }

    const std::uint16_t new_free_space_end = static_cast<std::uint16_t>(
        free_space_end() - bytes.size());
    std::memcpy(page_.data() + new_free_space_end, bytes.data(), bytes.size());
    set_slot(current_slot_count,
             Slot{new_free_space_end, static_cast<std::uint16_t>(bytes.size())});
    write_uint16(page_, kSlotCountOffset, static_cast<std::uint16_t>(current_slot_count + 1));
    set_free_space_end(new_free_space_end);
    slot_id = current_slot_count;
    return true;
}

bool TablePage::update_tuple(const Tuple& tuple, const Schema& schema, std::uint16_t slot_id) {
    if (!has_valid_header() || slot_id >= slot_count()) {
        return false;
    }

    const Slot slot = slot_at(slot_id);
    if (slot.length == 0) {
        return false;
    }
    const std::vector<std::byte> bytes = tuple.serialize(schema);
    if (bytes.size() > slot.length) {
        return false;
    }
    std::memcpy(page_.data() + slot.offset, bytes.data(), bytes.size());
    if (bytes.size() < slot.length) {
        std::memset(page_.data() + slot.offset + bytes.size(), 0, slot.length - bytes.size());
    }
    set_slot(slot_id, Slot{slot.offset, static_cast<std::uint16_t>(bytes.size())});
    return true;
}

std::optional<Tuple> TablePage::get_tuple(const Schema& schema, std::uint16_t slot_id) const {
    if (!has_valid_header() || slot_id >= slot_count()) {
        return std::nullopt;
    }

    const Slot slot = slot_at(slot_id);
    if (slot.length == 0) {
        return std::nullopt;
    }
    if (static_cast<std::size_t>(slot.offset) + slot.length > kPageSize) {
        throw std::runtime_error("Table page slot exceeds page bounds");
    }

    std::vector<std::byte> bytes(slot.length);
    std::memcpy(bytes.data(), page_.data() + slot.offset, slot.length);
    return Tuple::deserialize(schema, bytes);
}

bool TablePage::delete_tuple(std::uint16_t slot_id) {
    if (!has_valid_header() || slot_id >= slot_count()) {
        return false;
    }

    const Slot slot = slot_at(slot_id);
    if (slot.length == 0) {
        return false;
    }
    set_slot(slot_id, Slot{0, 0});
    return true;
}

std::uint16_t TablePage::slot_count() const {
    if (!has_valid_header()) {
        throw std::logic_error("Table page is not initialized");
    }
    std::uint16_t count = 0;
    read_uint16(page_, kSlotCountOffset, count);
    return count;
}

std::size_t TablePage::free_space() const {
    if (!has_valid_header()) {
        throw std::logic_error("Table page is not initialized");
    }
    return free_space_end() - (kHeaderSize + slot_count() * kSlotSize);
}

TablePage::Slot TablePage::slot_at(std::uint16_t slot_id) const {
    const std::size_t offset = kHeaderSize + slot_id * kSlotSize;
    Slot slot{};
    read_uint16(page_, offset, slot.offset);
    read_uint16(page_, offset + sizeof(std::uint16_t), slot.length);
    return slot;
}

void TablePage::set_slot(std::uint16_t slot_id, Slot slot) {
    const std::size_t offset = kHeaderSize + slot_id * kSlotSize;
    write_uint16(page_, offset, slot.offset);
    write_uint16(page_, offset + sizeof(std::uint16_t), slot.length);
}

std::uint16_t TablePage::free_space_end() const {
    std::uint16_t offset = 0;
    read_uint16(page_, kFreeSpaceEndOffset, offset);
    return offset;
}

void TablePage::set_free_space_end(std::uint16_t offset) {
    write_uint16(page_, kFreeSpaceEndOffset, offset);
}

bool TablePage::has_valid_header() const {
    const std::uint16_t end = free_space_end();
    return end >= kHeaderSize && end <= kPageSize;
}

}  // namespace minidb
