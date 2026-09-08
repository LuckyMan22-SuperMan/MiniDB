#include "minidb/schema.h"

#include <stdexcept>
#include <utility>

namespace minidb {

Column::Column(std::string name, Type type) : name_(std::move(name)), type_(type) {
    if (name_.empty()) {
        throw std::invalid_argument("Column name cannot be empty");
    }
}

const std::string& Column::name() const noexcept {
    return name_;
}

Type Column::type() const noexcept {
    return type_;
}

Schema::Schema(std::vector<Column> columns) : columns_(std::move(columns)) {}

std::size_t Schema::column_count() const noexcept {
    return columns_.size();
}

const Column& Schema::column(std::size_t index) const {
    if (index >= columns_.size()) {
        throw std::out_of_range("Column index is out of range");
    }
    return columns_[index];
}

std::size_t Schema::column_index(const std::string& name) const {
    for (std::size_t index = 0; index < columns_.size(); ++index) {
        if (columns_[index].name() == name) {
            return index;
        }
    }
    throw std::out_of_range("Unknown column: " + name);
}

}  // namespace minidb
