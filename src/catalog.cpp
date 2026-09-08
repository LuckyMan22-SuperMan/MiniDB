#include "minidb/catalog.h"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace minidb {

namespace {

constexpr std::uint32_t kCatalogMagic = 0x4D444243;
constexpr std::uint32_t kCatalogVersion = 1;
constexpr std::uint32_t kMaxCatalogEntries = 1'000'000;

void write_uint32(std::ostream& output, std::uint32_t value) {
    for (std::size_t shift = 0; shift < 32; shift += 8) {
        output.put(static_cast<char>((value >> shift) & 0xffU));
    }
    if (!output) {
        throw std::runtime_error("Unable to write catalog metadata");
    }
}

std::uint32_t read_uint32(std::istream& input) {
    std::uint32_t value = 0;
    for (std::size_t shift = 0; shift < 32; shift += 8) {
        const int character = input.get();
        if (character == std::char_traits<char>::eof()) {
            throw std::runtime_error("Catalog metadata is truncated");
        }
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(character)) << shift;
    }
    return value;
}

void write_string(std::ostream& output, const std::string& value) {
    if (value.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("Catalog string is too large");
    }
    write_uint32(output, static_cast<std::uint32_t>(value.size()));
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
    if (!output) {
        throw std::runtime_error("Unable to write catalog string");
    }
}

std::string read_string(std::istream& input) {
    const std::uint32_t length = read_uint32(input);
    if (length > kMaxCatalogEntries * 1024U) {
        throw std::runtime_error("Catalog string is unreasonably large");
    }
    std::string value(length, '\0');
    input.read(value.data(), static_cast<std::streamsize>(length));
    if (input.gcount() != static_cast<std::streamsize>(length)) {
        throw std::runtime_error("Catalog string is truncated");
    }
    return value;
}

}  // namespace

Catalog::Catalog(const std::filesystem::path& catalog_path) : catalog_path_(catalog_path) {
    load();
}

std::uint32_t Catalog::create_table(std::string name, Schema schema,
                                    std::vector<PageId> page_ids) {
    if (name.empty()) {
        throw std::invalid_argument("Table name cannot be empty");
    }
    if (get_table(name).has_value()) {
        throw std::invalid_argument("Table already exists: " + name);
    }

    const std::uint32_t table_id = next_table_id_++;
    tables_.push_back(TableMetadata{table_id, std::move(name), std::move(schema),
                                    std::move(page_ids)});
    save();
    return table_id;
}

std::optional<TableMetadata> Catalog::get_table(const std::string& name) const {
    for (const TableMetadata& table : tables_) {
        if (table.name == name) {
            return table;
        }
    }
    return std::nullopt;
}

std::vector<std::string> Catalog::table_names() const {
    std::vector<std::string> names;
    names.reserve(tables_.size());
    for (const TableMetadata& table : tables_) {
        names.push_back(table.name);
    }
    return names;
}

bool Catalog::update_table_pages(const std::string& name, std::vector<PageId> page_ids) {
    for (TableMetadata& table : tables_) {
        if (table.name == name) {
            table.page_ids = std::move(page_ids);
            save();
            return true;
        }
    }
    return false;
}

void Catalog::load() {
    tables_.clear();
    next_table_id_ = 0;

    std::ifstream input(catalog_path_, std::ios::binary);
    if (!input.is_open()) {
        return;
    }

    if (read_uint32(input) != kCatalogMagic || read_uint32(input) != kCatalogVersion) {
        throw std::runtime_error("Catalog has an unsupported format");
    }
    next_table_id_ = read_uint32(input);
    const std::uint32_t table_count = read_uint32(input);
    if (table_count > kMaxCatalogEntries) {
        throw std::runtime_error("Catalog contains too many tables");
    }

    for (std::uint32_t table_index = 0; table_index < table_count; ++table_index) {
        const std::uint32_t table_id = read_uint32(input);
        const std::string name = read_string(input);
        const std::uint32_t column_count = read_uint32(input);
        if (column_count > kMaxCatalogEntries) {
            throw std::runtime_error("Catalog contains too many columns");
        }

        std::vector<Column> columns;
        columns.reserve(column_count);
        for (std::uint32_t column_index = 0; column_index < column_count; ++column_index) {
            const std::string column_name = read_string(input);
            const auto encoded_type = static_cast<Type>(read_uint32(input));
            if (encoded_type != Type::Int && encoded_type != Type::Varchar &&
                encoded_type != Type::Boolean) {
                throw std::runtime_error("Catalog contains an invalid column type");
            }
            columns.emplace_back(column_name, encoded_type);
        }

        const std::uint32_t page_count = read_uint32(input);
        if (page_count > kMaxCatalogEntries) {
            throw std::runtime_error("Catalog contains too many table pages");
        }
        std::vector<PageId> page_ids;
        page_ids.reserve(page_count);
        for (std::uint32_t page_index = 0; page_index < page_count; ++page_index) {
            page_ids.push_back(static_cast<PageId>(read_uint32(input)));
        }

        tables_.push_back(TableMetadata{table_id, name, Schema(std::move(columns)),
                                        std::move(page_ids)});
    }
}

void Catalog::save() const {
    const auto temporary_path = catalog_path_.string() + ".tmp";
    std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        throw std::runtime_error("Unable to create catalog file: " + catalog_path_.string());
    }

    write_uint32(output, kCatalogMagic);
    write_uint32(output, kCatalogVersion);
    write_uint32(output, next_table_id_);
    write_uint32(output, static_cast<std::uint32_t>(tables_.size()));
    for (const TableMetadata& table : tables_) {
        write_uint32(output, table.table_id);
        write_string(output, table.name);
        write_uint32(output, static_cast<std::uint32_t>(table.schema.column_count()));
        for (std::size_t index = 0; index < table.schema.column_count(); ++index) {
            const Column& column = table.schema.column(index);
            write_string(output, column.name());
            write_uint32(output, static_cast<std::uint32_t>(column.type()));
        }
        write_uint32(output, static_cast<std::uint32_t>(table.page_ids.size()));
        for (const PageId page_id : table.page_ids) {
            write_uint32(output, static_cast<std::uint32_t>(page_id));
        }
    }
    output.close();
    if (!output) {
        throw std::runtime_error("Unable to finalize catalog file");
    }

    std::error_code error;
    std::filesystem::rename(temporary_path, catalog_path_, error);
    if (error) {
        std::filesystem::remove(catalog_path_, error);
        error.clear();
        std::filesystem::rename(temporary_path, catalog_path_, error);
        if (error) {
            throw std::runtime_error("Unable to replace catalog file: " + error.message());
        }
    }
}

}  // namespace minidb
