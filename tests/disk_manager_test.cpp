#include "minidb/disk_manager.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>

using namespace std;

int main() {
    const auto database_path = filesystem::temp_directory_path() / "minidb_phase1_test.db";
    filesystem::remove(database_path);

    {
        minidb::DiskManager disk_manager(database_path);
        assert(disk_manager.page_count() == 0);

        const auto page_id = disk_manager.allocate_page();
        assert(page_id == 0);
        assert(disk_manager.page_count() == 1);

        minidb::Page page;
        page.set_id(page_id);
        const string message = "persistent page data";
        for (size_t index = 0; index < message.size(); ++index) {
            page.data()[index] = static_cast<byte>(message[index]);
        }
        disk_manager.write_page(page_id, page);
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::Page page;
        disk_manager.read_page(0, page);

        const string expected = "persistent page data";
        for (size_t index = 0; index < expected.size(); ++index) {
            assert(page.data()[index] == static_cast<byte>(expected[index]));
        }
        assert(page.id() == 0);

        bool rejected_invalid_page = false;
        try {
            disk_manager.read_page(1, page);
        } catch (const out_of_range&) {
            rejected_invalid_page = true;
        }
        assert(rejected_invalid_page);
    }

    filesystem::remove(database_path);
    return 0;
}
