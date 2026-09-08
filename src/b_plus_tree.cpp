#include "minidb/b_plus_tree.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace minidb {

using namespace std;

namespace {

struct Entry { int32_t key; RID rid; };

constexpr uint32_t kLeafMagic = 0x4D44424C;
constexpr size_t kMagicOffset = 0;
constexpr size_t kNodeTypeOffset = 4;
constexpr size_t kKeyCountOffset = 6;
constexpr size_t kNextLeafOffset = 8;
constexpr size_t kHeaderSize = 12;
constexpr size_t kLeafEntrySize = sizeof(int32_t) + sizeof(PageId) + sizeof(uint16_t);
constexpr size_t kInternalHeaderSize = 16;
constexpr size_t kInternalEntrySize = sizeof(int32_t) + sizeof(PageId);
constexpr size_t kLeafCapacity = (kPageSize - kHeaderSize) / kLeafEntrySize;
constexpr size_t kInternalCapacity = (kPageSize - kInternalHeaderSize) / kInternalEntrySize;

uint32_t read_uint32(const Page& page, size_t offset) {
    uint32_t value = 0;
    for (size_t shift = 0; shift < 32; shift += 8) value |= static_cast<uint32_t>(to_integer<unsigned char>(page.data()[offset++])) << shift;
    return value;
}

void write_uint32(Page& page, size_t offset, uint32_t value) {
    for (size_t shift = 0; shift < 32; shift += 8) page.data()[offset++] = static_cast<byte>((value >> shift) & 0xffU);
}

uint16_t read_uint16(const Page& page, size_t offset) {
    return static_cast<uint16_t>(to_integer<unsigned char>(page.data()[offset])) |
           static_cast<uint16_t>(to_integer<unsigned char>(page.data()[offset + 1])) << 8;
}

void write_uint16(Page& page, size_t offset, uint16_t value) {
    page.data()[offset] = static_cast<byte>(value & 0xffU);
    page.data()[offset + 1] = static_cast<byte>((value >> 8) & 0xffU);
}

int32_t read_int32(const Page& page, size_t offset) { return static_cast<int32_t>(read_uint32(page, offset)); }
void write_int32(Page& page, size_t offset, int32_t value) { write_uint32(page, offset, static_cast<uint32_t>(value)); }
size_t leaf_offset(size_t index) { return kHeaderSize + index * kLeafEntrySize; }
size_t internal_offset(size_t index) { return kInternalHeaderSize + index * kInternalEntrySize; }
int32_t leaf_key(const Page& page, size_t index) { return read_int32(page, leaf_offset(index)); }
RID leaf_rid(const Page& page, size_t index) {
    const size_t offset = leaf_offset(index) + sizeof(int32_t);
    return RID{read_int32(page, offset), read_uint16(page, offset + sizeof(PageId))};
}
void write_leaf_entry(Page& page, size_t index, Entry entry) {
    const size_t offset = leaf_offset(index);
    write_int32(page, offset, entry.key);
    write_int32(page, offset + sizeof(int32_t), entry.rid.page_id);
    write_uint16(page, offset + sizeof(int32_t) + sizeof(PageId), entry.rid.slot_id);
}
void initialize_leaf(Page& page) {
    page.reset(); write_uint32(page, kMagicOffset, kLeafMagic); page.data()[kNodeTypeOffset] = byte{1};
    write_uint16(page, kKeyCountOffset, 0); write_int32(page, kNextLeafOffset, kInvalidPageId);
}
void initialize_internal(Page& page, PageId first_child) {
    page.reset(); write_uint32(page, kMagicOffset, kLeafMagic); page.data()[kNodeTypeOffset] = byte{2};
    write_uint16(page, kKeyCountOffset, 0); write_int32(page, kInternalHeaderSize - sizeof(PageId), first_child);
}
void validate_node(const Page& page) {
    if (read_uint32(page, kMagicOffset) != kLeafMagic ||
        (page.data()[kNodeTypeOffset] != byte{1} && page.data()[kNodeTypeOffset] != byte{2}))
        throw runtime_error("B+ Tree page is invalid");
}
vector<Entry> read_leaf_entries(const Page& page) {
    vector<Entry> entries;
    const uint16_t count = read_uint16(page, kKeyCountOffset);
    if (count > kLeafCapacity) throw runtime_error("B+ Tree leaf contains too many keys");
    for (size_t index = 0; index < count; ++index) entries.push_back(Entry{leaf_key(page, index), leaf_rid(page, index)});
    return entries;
}
void write_leaf_entries(Page& page, const vector<Entry>& entries, PageId next_leaf) {
    initialize_leaf(page);
    for (size_t index = 0; index < entries.size(); ++index) write_leaf_entry(page, index, entries[index]);
    write_uint16(page, kKeyCountOffset, static_cast<uint16_t>(entries.size()));
    write_int32(page, kNextLeafOffset, next_leaf);
}
PageId internal_first_child(const Page& page) { return read_int32(page, kInternalHeaderSize - sizeof(PageId)); }
void write_internal_entry(Page& page, size_t index, int32_t key, PageId child) {
    const size_t offset = internal_offset(index);
    write_int32(page, offset, key); write_int32(page, offset + sizeof(int32_t), child);
}
int32_t internal_key(const Page& page, size_t index) { return read_int32(page, internal_offset(index)); }
PageId internal_child(const Page& page, size_t index) { return read_int32(page, internal_offset(index) + sizeof(int32_t)); }
PageId find_child(const Page& page, int32_t key) {
    PageId child = internal_first_child(page);
    const uint16_t count = read_uint16(page, kKeyCountOffset);
    for (size_t index = 0; index < count; ++index) {
        if (key < internal_key(page, index)) break;
        child = internal_child(page, index);
    }
    return child;
}

}  // namespace

BPlusTree::BPlusTree(BufferPoolManager& buffer_pool, PageId root_page_id)
    : buffer_pool_(buffer_pool), root_page_id_(root_page_id) {}

unique_ptr<BPlusTree> BPlusTree::create(BufferPoolManager& buffer_pool, PageId& root_page_id) {
    Page* page = buffer_pool.new_page(root_page_id);
    if (page == nullptr) { root_page_id = kInvalidPageId; return nullptr; }
    initialize_leaf(*page);
    const bool unpinned = buffer_pool.unpin_page(root_page_id, true); (void)unpinned;
    return make_unique<BPlusTree>(buffer_pool, root_page_id);
}

bool BPlusTree::insert(int32_t key, RID rid) {
    Page* root = buffer_pool_.fetch_page(root_page_id_);
    if (root == nullptr) return false;
    validate_node(*root);
    if (root->data()[kNodeTypeOffset] == byte{1}) {
        vector<Entry> entries = read_leaf_entries(*root);
        auto position = lower_bound(entries.begin(), entries.end(), key, [](Entry entry, int32_t value) { return entry.key < value; });
        if (position != entries.end() && position->key == key) { const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)unpinned; return false; }
        entries.insert(position, Entry{key, rid});
        if (entries.size() <= kLeafCapacity) {
            write_leaf_entries(*root, entries, kInvalidPageId);
            const bool unpinned = buffer_pool_.unpin_page(root_page_id_, true); (void)unpinned; return true;
        }
        if (kInternalCapacity == 0) throw runtime_error("B+ Tree internal page has no capacity");
        PageId left_id = kInvalidPageId, right_id = kInvalidPageId;
        Page* left = buffer_pool_.new_page(left_id); Page* right = buffer_pool_.new_page(right_id);
        if (left == nullptr || right == nullptr) { const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)unpinned; return false; }
        const size_t middle = entries.size() / 2;
        vector<Entry> left_entries(entries.begin(), entries.begin() + middle);
        vector<Entry> right_entries(entries.begin() + middle, entries.end());
        write_leaf_entries(*left, left_entries, right_id); write_leaf_entries(*right, right_entries, kInvalidPageId);
        initialize_internal(*root, left_id); write_internal_entry(*root, 0, right_entries.front().key, right_id);
        write_uint16(*root, kKeyCountOffset, 1);
        const bool left_unpinned = buffer_pool_.unpin_page(left_id, true); (void)left_unpinned;
        const bool right_unpinned = buffer_pool_.unpin_page(right_id, true); (void)right_unpinned;
        const bool root_unpinned = buffer_pool_.unpin_page(root_page_id_, true); (void)root_unpinned;
        return true;
    }
    const PageId child_id = find_child(*root, key);
    const uint16_t root_count = read_uint16(*root, kKeyCountOffset);
    if (root_count >= kInternalCapacity) {
        const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)unpinned;
        return false;
    }
    const bool root_unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)root_unpinned;
    Page* child = buffer_pool_.fetch_page(child_id);
    if (child == nullptr) return false;
    vector<Entry> entries = read_leaf_entries(*child);
    auto position = lower_bound(entries.begin(), entries.end(), key, [](Entry entry, int32_t value) { return entry.key < value; });
    if (position != entries.end() && position->key == key) { const bool unpinned = buffer_pool_.unpin_page(child_id, false); (void)unpinned; return false; }
    entries.insert(position, Entry{key, rid});
    if (entries.size() <= kLeafCapacity) {
        const int32_t next = read_int32(*child, kNextLeafOffset); write_leaf_entries(*child, entries, next);
        const bool unpinned = buffer_pool_.unpin_page(child_id, true); (void)unpinned; return true;
    }
    PageId right_id = kInvalidPageId; Page* right = buffer_pool_.new_page(right_id);
    if (right == nullptr) { const bool unpinned = buffer_pool_.unpin_page(child_id, false); (void)unpinned; return false; }
    const size_t middle = entries.size() / 2;
    vector<Entry> left_entries(entries.begin(), entries.begin() + middle);
    vector<Entry> right_entries(entries.begin() + middle, entries.end());
    const int32_t next = read_int32(*child, kNextLeafOffset);
    write_leaf_entries(*child, left_entries, right_id); write_leaf_entries(*right, right_entries, next);
    const bool child_unpinned = buffer_pool_.unpin_page(child_id, true); (void)child_unpinned;
    const bool right_unpinned = buffer_pool_.unpin_page(right_id, true); (void)right_unpinned;
    Page* internal = buffer_pool_.fetch_page(root_page_id_); validate_node(*internal);
    const uint16_t count = read_uint16(*internal, kKeyCountOffset);
    if (count >= kInternalCapacity) { const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)unpinned; return false; }
    size_t insertion = 0;
    while (insertion < count && internal_key(*internal, insertion) < right_entries.front().key) ++insertion;
    for (size_t index = count; index > insertion; --index) write_internal_entry(*internal, index, internal_key(*internal, index - 1), internal_child(*internal, index - 1));
    write_internal_entry(*internal, insertion, right_entries.front().key, right_id);
    write_uint16(*internal, kKeyCountOffset, static_cast<uint16_t>(count + 1));
    const bool internal_unpinned = buffer_pool_.unpin_page(root_page_id_, true); (void)internal_unpinned;
    return true;
}

optional<RID> BPlusTree::search(int32_t key) const {
    Page* root = buffer_pool_.fetch_page(root_page_id_); if (root == nullptr) return nullopt;
    validate_node(*root); PageId leaf_id = root_page_id_;
    if (root->data()[kNodeTypeOffset] == byte{2}) leaf_id = find_child(*root, key);
    const bool root_unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)root_unpinned;
    Page* leaf = leaf_id == root_page_id_ ? buffer_pool_.fetch_page(leaf_id) : buffer_pool_.fetch_page(leaf_id);
    if (leaf == nullptr) return nullopt; validate_node(*leaf);
    for (const Entry entry : read_leaf_entries(*leaf)) if (entry.key == key) { const bool unpinned = buffer_pool_.unpin_page(leaf_id, false); (void)unpinned; return entry.rid; }
    const bool unpinned = buffer_pool_.unpin_page(leaf_id, false); (void)unpinned; return nullopt;
}

size_t BPlusTree::size() const {
    Page* root = buffer_pool_.fetch_page(root_page_id_); if (root == nullptr) return 0;
    validate_node(*root); size_t total = 0;
    if (root->data()[kNodeTypeOffset] == byte{1}) total = read_uint16(*root, kKeyCountOffset);
    else {
        total = read_uint16(*root, kKeyCountOffset) ? 0 : 0;
        PageId child = internal_first_child(*root); const uint16_t count = read_uint16(*root, kKeyCountOffset);
        for (size_t index = 0; index <= count; ++index) {
            if (index > 0) child = internal_child(*root, index - 1);
            Page* leaf = buffer_pool_.fetch_page(child); if (leaf == nullptr) continue;
            total += read_uint16(*leaf, kKeyCountOffset); const bool unpinned = buffer_pool_.unpin_page(child, false); (void)unpinned;
        }
    }
    const bool unpinned = buffer_pool_.unpin_page(root_page_id_, false); (void)unpinned; return total;
}

PageId BPlusTree::root_page_id() const noexcept { return root_page_id_; }

}  // namespace minidb
