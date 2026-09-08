#include "minidb/buffer_pool_manager.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>

using namespace std;

namespace {

void write_text(minidb::Page& page, const string& text) {
    for (size_t index = 0; index < text.size(); ++index) {
        page.data()[index] = static_cast<byte>(text[index]);
    }
}

string read_text(const minidb::Page& page, size_t length) {
    string text(length, '\0');
    for (size_t index = 0; index < length; ++index) {
        text[index] = static_cast<char>(page.data()[index]);
    }
    return text;
}

}  // namespace

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase2_test.db";
    filesystem::remove(database_path);

    minidb::DiskManager disk_manager(database_path);
    minidb::BufferPoolManager buffer_pool(disk_manager, 2);

    minidb::PageId first_page_id = minidb::kInvalidPageId;
    minidb::Page* first_page = buffer_pool.new_page(first_page_id);
    assert(first_page != nullptr);
    write_text(*first_page, "first page");
    assert(buffer_pool.unpin_page(first_page_id, true));

    minidb::PageId second_page_id = minidb::kInvalidPageId;
    minidb::Page* second_page = buffer_pool.new_page(second_page_id);
    assert(second_page != nullptr);
    write_text(*second_page, "second page");
    assert(buffer_pool.unpin_page(second_page_id, true));

    minidb::Page* fetched_first = buffer_pool.fetch_page(first_page_id);
    assert(fetched_first != nullptr);
    assert(read_text(*fetched_first, 10) == "first page");
    assert(buffer_pool.unpin_page(first_page_id, false));

    minidb::PageId third_page_id = minidb::kInvalidPageId;
    minidb::Page* third_page = buffer_pool.new_page(third_page_id);
    assert(third_page != nullptr);
    assert(third_page_id != first_page_id);
    assert(third_page_id != second_page_id);
    assert(buffer_pool.unpin_page(third_page_id, false));

    buffer_pool.flush_all_pages();
    {
        minidb::DiskManager reopened_disk_manager(database_path);
        minidb::Page persisted_page;
        reopened_disk_manager.read_page(first_page_id, persisted_page);
        assert(read_text(persisted_page, 10) == "first page");
    }

    minidb::BufferPoolManager one_frame_pool(disk_manager, 1);
    minidb::Page* pinned_page = one_frame_pool.fetch_page(first_page_id);
    assert(pinned_page != nullptr);
    assert(one_frame_pool.fetch_page(second_page_id) == nullptr);
    assert(one_frame_pool.unpin_page(first_page_id, false));
    assert(one_frame_pool.fetch_page(second_page_id) != nullptr);
    assert(one_frame_pool.unpin_page(second_page_id, false));

    minidb::Page* page_to_delete = buffer_pool.fetch_page(third_page_id);
    assert(page_to_delete != nullptr);
    assert(!buffer_pool.delete_page(third_page_id));
    assert(buffer_pool.unpin_page(third_page_id, false));
    assert(buffer_pool.delete_page(third_page_id));

    filesystem::remove(database_path);
    return 0;
}
