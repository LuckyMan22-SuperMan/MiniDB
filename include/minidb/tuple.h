#pragma once

#include "minidb/schema.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace minidb {

class Tuple {
public:
    explicit Tuple(std::vector<Value> values);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const Value& value_at(std::size_t index) const;
    [[nodiscard]] const std::vector<Value>& values() const noexcept;

    [[nodiscard]] std::vector<std::byte> serialize(const Schema& schema) const;
    [[nodiscard]] static Tuple deserialize(const Schema& schema, const std::vector<std::byte>& bytes);

private:
    std::vector<Value> values_;
};

}  // namespace minidb
