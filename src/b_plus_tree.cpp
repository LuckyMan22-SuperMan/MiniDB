#include "minidb/b_plus_tree.h"

#include <cstring>
#include <stdexcept>

namespace minidb {

using namespace std;

namespace {

constexpr uint32_t kLeafMagic = 0x4D44424C;
constexpr size_t kMagicOffset = 0;
constexpr size_t kNodeTypeOffset = 4;
constexpr size_t kKeyCountOffset = 6;
constexpr size_t kNextLeafOffset = 8;
constexpr size_t kHeaderSize = 12;
constexpr size_t kEntrySize = sizeof(int32_t) + sizeof(PageId) + sizeof(uint16_t);
constexpr size_t kLeafCapacity = (kPageSize - kHeaderSize) / kEntrySize;

uint32_t read_uint32(const Page& page, size_t offset) {
    uint32_t value = 0;
    for (size_t shift = 0; shift < 32; shift += 8) {
        value |= static_cast<uint32_t>(to_integer<unsigned char>(page.data()[offset++])) << shift;
    }
    return value;
}

void write_uint32(Page& page, size_t offset, uint32_t value) {
    for (size_t shift = 0; shift < 32; shift += 8) {
        page.data()[offset++] = static_cast<byte>((value >> shift) & 0xffU);
    }
}

uint16_t read_uint16(const Page& page, size_t offset) {
    return static_cast<uint16_t>(to_integer<unsigned char>(page.data()[offset])) |
           static_cast<uint16_t>(to_integer<unsigned char>(page.data()[offset + 1])) << 8;
}

void write_uint16(Page& page, size_t offset, uint16_t value) {
    page.data()[offset] = static_cast<byte>(value & 0xffU);
    page.data()[offset + 1] = static_cast<byte>((value >> 8) & 0xffU);
}

int32_t read_int32(const Page& page, size_t offset) {
    return static_cast<int32_t>(read_uint32(page, offset));
}

void write_int32(Page& page, size_t offset, int32_t value) {
    write_uint32(page, offset, static_cast<uint32_t>(value));
}

size_t entry_offset(size_t index) {
    return kHeaderSize + index * kEntrySize;
}

int32_t entry_key(const Page& page, size_t index) {
    return read_int32(page, entry_offset(index));
}

RID entry_rid(const Page& page, size_t index) {
    const size_t offset = entry_offset(index) + sizeof(int32_t);
    return RID{read_int32(page, offset), read_uint16(page, offset + sizeof(PageId))};
}

void write_entry(Page& page, size_t index, int32_t key, RID rid) {
    const size_t offset = entry_offset(index);
    write_int32(page, offset, key);
    write_int32(page, offset + sizeof(int32_t), rid.page_id);
    write_uint16(page, offset + sizeof(int32_t) + sizeof(PageId), rid.slot_id);
}

void initialize_leaf(Page& page) {
    page.reset();
    write_uint32(page, kMagicOffset, kLeafMagic);
    page.data()[kNodeTypeOffset] = byte{1};
    write_uint16(page, kKeyCountOffset, 0);
    write_int32(page, kNextLeafOffset, kInvalidPageId);
}

void validate_leaf(const Page& page) {
    if (read_uint32(page, kMagicOffset) != kLeafMagic ||
        page.data()[kNodeTypeOffset] != byte{1}) {
        throw runtime_error("B+ Tree root page is not a valid leaf");
    }
    if (read_uint16(page, kKeyCountOffset) > kLeafCapacity) {
        throw runtime_error("B+ Tree leaf contains too many keys");
    }
}

}  // namespace

BPlusTree::BPlusTree(BufferPoolManager& buffer_pool, PageId root_page_id)
    : buffer_pool_(buffer_pool), root_page_id_(root_page_id) {}

unique_ptr<BPlusTree> BPlusTree::create(BufferPoolManager& buffer_pool, PageId& root_page_id) {
    Page* page = buffer_pool.new_page(root_page_id);
    if (page == nullptr) {
        root_page_id = kInvalidPageId;
        return nullptr;
    }
    initialize_leaf(*page);
    const bool unpinned = buffer_pool.unpin_page(root_page_id, true);
    (void)unpinned;
    return make_unique<BPlusTree>(buffer_pool, root_page_id);
}

bool BPlusTree::insert(int32_t key, RID rid) {
    Page* page = buffer_pool_.fetch_page(root_page_id_);
    if (page == nullptr) {
        return false;
    }

    validate_leaf(*page);
    const uint16_t count = read_uint16(*page, kKeyCountOffset);
    size_t insertion_index = 0;
    while (insertion_index < count && entry_key(*page, insertion_index) < key) {
        ++insertion_index;
    }
    if (insertion_index < count && entry_key(*page, insertion_index) == key) {
        const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false);
        (void)unpinned;
        return false;
    }
    if (count >= kLeafCapacity) {
        const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false);
        (void)unpinned;
        return false;
    }

    for (size_t index = count; index > insertion_index; --index) {
        write_entry(*page, index, entry_key(*page, index - 1), entry_rid(*page, index - 1));
    }
    write_entry(*page, insertion_index, key, rid);
    write_uint16(*page, kKeyCountOffset, static_cast<uint16_t>(count + 1));
    const bool unpinned = buffer_pool_.unpin_page(root_page_id_, true);
    (void)unpinned;
    return true;
}

optional<RID> BPlusTree::search(int32_t key) const {
    Page* page = buffer_pool_.fetch_page(root_page_id_);
    if (page == nullptr) {
        return nullopt;
    }

    validate_leaf(*page);
    const uint16_t count = read_uint16(*page, kKeyCountOffset);
    for (size_t index = 0; index < count; ++index) {
        if (entry_key(*page, index) == key) {
            const RID rid = entry_rid(*page, index);
            const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false);
            (void)unpinned;
            return rid;
        }
    }
    const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false);
    (void)unpinned;
    return nullopt;
}

size_t BPlusTree::size() const {
    Page* page = buffer_pool_.fetch_page(root_page_id_);
    if (page == nullptr) {
        return 0;
    }
    validate_leaf(*page);
    const size_t count = read_uint16(*page, kKeyCountOffset);
    const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false);
    (void)unpinned;
    return count;
}

PageId BPlusTree::root_page_id() const noexcept {
    return root_page_id_;
}

}  // namespace minidb
