#include "minidb/disk_manager.h"

#include <cassert>
#include <cstddef>
#include <filesystem>
#include <string>

int main() {
    const auto database_path = std::filesystem::temp_directory_path() / "minidb_phase1_test.db";
    std::filesystem::remove(database_path);

    {
        minidb::DiskManager disk_manager(database_path);
        assert(disk_manager.page_count() == 0);

        const auto page_id = disk_manager.allocate_page();
        assert(page_id == 0);
        assert(disk_manager.page_count() == 1);

        minidb::Page page;
        page.set_id(page_id);
        const std::string message = "persistent page data";
        for (std::size_t index = 0; index < message.size(); ++index) {
            page.data()[index] = static_cast<std::byte>(message[index]);
        }
        disk_manager.write_page(page_id, page);
    }

    {
        minidb::DiskManager disk_manager(database_path);
        minidb::Page page;
        disk_manager.read_page(0, page);

        const std::string expected = "persistent page data";
        for (std::size_t index = 0; index < expected.size(); ++index) {
            assert(page.data()[index] == static_cast<std::byte>(expected[index]));
        }
        assert(page.id() == 0);

        bool rejected_invalid_page = false;
        try {
            disk_manager.read_page(1, page);
        } catch (const std::out_of_range&) {
            rejected_invalid_page = true;
        }
        assert(rejected_invalid_page);
    }

    std::filesystem::remove(database_path);
    return 0;
}
