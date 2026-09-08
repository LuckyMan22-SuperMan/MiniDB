#include "minidb/b_plus_tree.h"

#include <cassert>
#include <filesystem>

using namespace std;

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase12_btree.db";
    filesystem::remove(database_path);

    minidb::PageId root_page_id = minidb::kInvalidPageId;
    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 2);
        const auto tree = minidb::BPlusTree::create(buffer_pool, root_page_id);
        assert(tree != nullptr);
        assert(tree->insert(20, minidb::RID{5, 2}));
        assert(tree->insert(10, minidb::RID{3, 1}));
        assert(tree->insert(30, minidb::RID{8, 0}));
        assert(!tree->insert(20, minidb::RID{9, 9}));
        assert(tree->size() == 3);
        assert(tree->search(10)->page_id == 3);
        assert(tree->search(20)->slot_id == 2);
        assert(!tree->search(99).has_value());
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 2);
        const minidb::BPlusTree tree(buffer_pool, root_page_id);
        assert(tree.size() == 3);
        assert(tree.search(30)->page_id == 8);
        assert(tree.search(10)->slot_id == 1);
    }

    filesystem::remove(database_path);
    return 0;
}
