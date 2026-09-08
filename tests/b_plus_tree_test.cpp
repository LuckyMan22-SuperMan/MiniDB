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
        minidb::BufferPoolManager buffer_pool(disk_manager, 8);
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
        for (int32_t key = 0; key < 500; ++key) {
            if (key != 10 && key != 20 && key != 30) {
                assert(tree->insert(key, minidb::RID{key + 100, 1}));
            }
        }
        assert(tree->size() == 500);
        assert(tree->search(0)->page_id == 100);
        assert(tree->search(499)->page_id == 599);
        for (int32_t key = 0; key < 250; ++key) {
            assert(tree->remove(key));
        }
        assert(tree->size() == 250);
        assert(!tree->search(0).has_value());
        assert(tree->search(250)->page_id == 350);
        buffer_pool.flush_all_pages();
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::BufferPoolManager buffer_pool(disk_manager, 8);
        minidb::BPlusTree tree(buffer_pool, root_page_id);
        assert(tree.size() == 250);
        assert(!tree.search(0).has_value());
        assert(tree.search(250)->page_id == 350);
        assert(tree.search(499)->slot_id == 1);
        for (int32_t key = 250; key < 500; ++key) {
            assert(tree.remove(key));
        }
        assert(tree.size() == 0);
        assert(tree.insert(42, minidb::RID{7, 7}));
        assert(tree.search(42)->page_id == 7);
    }

    filesystem::remove(database_path);
    return 0;
}
