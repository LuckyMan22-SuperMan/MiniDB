#pragma once

#include "minidb/value.h"

#include <cstddef>
#include <string>
#include <vector>

namespace minidb {

class Column {
public:
    Column(std::string name, Type type);

    [[nodiscard]] const std::string& name() const noexcept;
    [[nodiscard]] Type type() const noexcept;

private:
    std::string name_;
    Type type_;
};

class Schema {
public:
    explicit Schema(std::vector<Column> columns);

    [[nodiscard]] std::size_t column_count() const noexcept;
    [[nodiscard]] const Column& column(std::size_t index) const;
    [[nodiscard]] std::size_t column_index(const std::string& name) const;

private:
    std::vector<Column> columns_;
};

}  // namespace minidb
