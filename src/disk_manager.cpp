#include "minidb/disk_manager.h"

#include <array>
#include <stdexcept>
#include <string>

namespace minidb {

using namespace std;

DiskManager::DiskManager(const filesystem::path& database_path)
    : database_path_(database_path) {
    database_file_.open(database_path_, ios::in | ios::out | ios::binary);
    if (!database_file_.is_open()) {
        ofstream create_file(database_path_, ios::binary);
        if (!create_file) {
                throw runtime_error("Unable to create database file: " + database_path_.string());
        }
        create_file.close();
        database_file_.open(database_path_, ios::in | ios::out | ios::binary);
    }

    if (!database_file_.is_open()) {
        throw runtime_error("Unable to open database file: " + database_path_.string());
    }
}

DiskManager::~DiskManager() {
    if (database_file_.is_open()) {
        database_file_.flush();
        database_file_.close();
    }
}

PageId DiskManager::allocate_page() {
    const PageId page_id = static_cast<PageId>(page_count());
    ensure_stream_is_open();

    Page empty_page;
    empty_page.reset();
    database_file_.clear();
    database_file_.seekp(static_cast<streamoff>(page_id) * kPageSize);
    database_file_.write(reinterpret_cast<const char*>(empty_page.data()), kPageSize);
    database_file_.flush();
    if (!database_file_) {
        throw runtime_error("Unable to allocate page " + to_string(page_id));
    }
    return page_id;
}

void DiskManager::read_page(PageId page_id, Page& page) {
    validate_page_id(page_id);
    ensure_stream_is_open();

    database_file_.clear();
    database_file_.seekg(static_cast<streamoff>(page_id) * kPageSize);
    database_file_.read(reinterpret_cast<char*>(page.data()), kPageSize);
    if (database_file_.gcount() != static_cast<streamsize>(kPageSize)) {
        throw runtime_error("Unable to read complete page " + to_string(page_id));
    }
    page.set_id(page_id);
}

void DiskManager::write_page(PageId page_id, const Page& page) {
    validate_page_id(page_id);
    ensure_stream_is_open();

    database_file_.clear();
    database_file_.seekp(static_cast<streamoff>(page_id) * kPageSize);
    database_file_.write(reinterpret_cast<const char*>(page.data()), kPageSize);
    database_file_.flush();
    if (!database_file_) {
        throw runtime_error("Unable to write page " + to_string(page_id));
    }
}

void DiskManager::deallocate_page(PageId page_id) {
    validate_page_id(page_id);
    Page empty_page;
    empty_page.reset();
    write_page(page_id, empty_page);
}

uint64_t DiskManager::page_count() const {
    ensure_stream_is_open();

    database_file_.clear();
    database_file_.seekg(0, ios::end);
    const auto file_size = database_file_.tellg();
    if (file_size < 0 || file_size % static_cast<streamoff>(kPageSize) != 0) {
        throw runtime_error("Database file has an invalid size");
    }
    return static_cast<uint64_t>(file_size / static_cast<streamoff>(kPageSize));
}

void DiskManager::validate_page_id(PageId page_id) const {
    if (page_id < 0 || static_cast<uint64_t>(page_id) >= page_count()) {
        throw out_of_range("Invalid page ID: " + to_string(page_id));
    }
}

void DiskManager::ensure_stream_is_open() const {
    if (!database_file_.is_open()) {
        throw runtime_error("Database file is not open: " + database_path_.string());
    }
}

}  // namespace minidb
