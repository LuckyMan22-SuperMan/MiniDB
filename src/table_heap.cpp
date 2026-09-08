#include "minidb/table_heap.h"

#include "minidb/table_page.h"

#include <utility>

namespace minidb {

TableHeap::TableHeap(BufferPoolManager& buffer_pool, Schema schema)
    : buffer_pool_(buffer_pool), schema_(std::move(schema)) {}

TableHeap::TableHeap(BufferPoolManager& buffer_pool, Schema schema,
                     std::vector<PageId> page_ids)
    : buffer_pool_(buffer_pool), schema_(std::move(schema)), page_ids_(std::move(page_ids)) {}

std::optional<RID> TableHeap::insert_tuple(const Tuple& tuple) {
    for (const PageId page_id : page_ids_) {
        Page* page = buffer_pool_.fetch_page(page_id);
        if (page == nullptr) {
            continue;
        }

        TablePage table_page(*page);
        std::uint16_t slot_id = 0;
        const bool inserted = table_page.insert_tuple(tuple, schema_, slot_id);
        const bool unpinned = buffer_pool_.unpin_page(page_id, inserted);
        (void)unpinned;
        if (inserted) {
            return RID{page_id, slot_id};
        }
    }

    PageId page_id = kInvalidPageId;
    Page* page = buffer_pool_.new_page(page_id);
    if (page == nullptr) {
        return std::nullopt;
    }

    TablePage table_page(*page);
    table_page.initialize();
    std::uint16_t slot_id = 0;
    const bool inserted = table_page.insert_tuple(tuple, schema_, slot_id);
    const bool unpinned = buffer_pool_.unpin_page(page_id, inserted);
    (void)unpinned;
    if (!inserted) {
        const bool deleted = buffer_pool_.delete_page(page_id);
        (void)deleted;
        return std::nullopt;
    }

    page_ids_.push_back(page_id);
    return RID{page_id, slot_id};
}

std::optional<Tuple> TableHeap::get_tuple(const RID& rid) {
    Page* page = buffer_pool_.fetch_page(rid.page_id);
    if (page == nullptr) {
        return std::nullopt;
    }

    TablePage table_page(*page);
    std::optional<Tuple> tuple = table_page.get_tuple(schema_, rid.slot_id);
    const bool unpinned = buffer_pool_.unpin_page(rid.page_id, false);
    (void)unpinned;
    return tuple;
}

std::vector<Tuple> TableHeap::scan() {
    std::vector<Tuple> tuples;
    for (const TableRecord& record : scan_records()) {
        tuples.push_back(record.tuple);
    }
    return tuples;
}

std::vector<TableRecord> TableHeap::scan_records() {
    std::vector<TableRecord> records;
    for (const PageId page_id : page_ids_) {
        Page* page = buffer_pool_.fetch_page(page_id);
        if (page == nullptr) {
            continue;
        }

        TablePage table_page(*page);
        const std::uint16_t slots = table_page.slot_count();
        for (std::uint16_t slot_id = 0; slot_id < slots; ++slot_id) {
            const std::optional<Tuple> tuple = table_page.get_tuple(schema_, slot_id);
            if (tuple.has_value()) {
                records.push_back(TableRecord{RID{page_id, slot_id}, *tuple});
            }
        }
        const bool unpinned = buffer_pool_.unpin_page(page_id, false);
        (void)unpinned;
    }
    return records;
}

bool TableHeap::update_tuple(const RID& rid, const Tuple& tuple) {
    Page* page = buffer_pool_.fetch_page(rid.page_id);
    if (page == nullptr) {
        return false;
    }

    TablePage table_page(*page);
    const bool updated = table_page.update_tuple(tuple, schema_, rid.slot_id);
    const bool unpinned = buffer_pool_.unpin_page(rid.page_id, updated);
    (void)unpinned;
    return updated;
}

bool TableHeap::delete_tuple(const RID& rid) {
    Page* page = buffer_pool_.fetch_page(rid.page_id);
    if (page == nullptr) {
        return false;
    }

    TablePage table_page(*page);
    const bool deleted = table_page.delete_tuple(rid.slot_id);
    const bool unpinned = buffer_pool_.unpin_page(rid.page_id, deleted);
    (void)unpinned;
    return deleted;
}

void TableHeap::flush() {
    buffer_pool_.flush_all_pages();
}

const Schema& TableHeap::schema() const noexcept {
    return schema_;
}

const std::vector<PageId>& TableHeap::page_ids() const noexcept {
    return page_ids_;
}

}  // namespace minidb
