#pragma once

#include "minidb/page.h"

#include <cstdint>
#include <filesystem>
#include <fstream>

namespace minidb {

class DiskManager {
public:
    explicit DiskManager(const std::filesystem::path& database_path);
    ~DiskManager();

    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    [[nodiscard]] PageId allocate_page();
    void read_page(PageId page_id, Page& page);
    void write_page(PageId page_id, const Page& page);
    void deallocate_page(PageId page_id);

    [[nodiscard]] std::uint64_t page_count() const;

private:
    void validate_page_id(PageId page_id) const;
    void ensure_stream_is_open() const;

    std::filesystem::path database_path_;
    mutable std::fstream database_file_;
};

}  // namespace minidb
