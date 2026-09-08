#pragma once

#include "minidb/page.h"
#include "minidb/schema.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace minidb {

struct TableMetadata {
    std::uint32_t table_id;
    std::string name;
    Schema schema;
    std::vector<PageId> page_ids;
};

struct IndexMetadata {
    std::string name;
    std::string table_name;
    std::string column_name;
    PageId root_page_id;
};

class Catalog {
public:
    explicit Catalog(const std::filesystem::path& catalog_path);

    [[nodiscard]] std::uint32_t create_table(std::string name, Schema schema,
                                              std::vector<PageId> page_ids = {});
    [[nodiscard]] std::optional<TableMetadata> get_table(const std::string& name) const;
    [[nodiscard]] std::vector<std::string> table_names() const;
    [[nodiscard]] bool update_table_pages(const std::string& name,
                                          std::vector<PageId> page_ids);
    [[nodiscard]] bool create_index(IndexMetadata metadata);
    [[nodiscard]] std::optional<IndexMetadata> get_index(const std::string& name) const;
    [[nodiscard]] std::vector<IndexMetadata> indexes_for_table(const std::string& table_name) const;

private:
    void load();
    void save() const;

    std::filesystem::path catalog_path_;
    std::uint32_t next_table_id_{0};
    std::vector<TableMetadata> tables_;
    std::vector<IndexMetadata> indexes_;
};

}  // namespace minidb
