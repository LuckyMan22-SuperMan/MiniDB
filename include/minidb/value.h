#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace minidb {

enum class Type : std::uint8_t {
    Int = 1,
    Varchar = 2,
    Boolean = 3,
};

class Value {
public:
    explicit Value(std::int32_t value);
    explicit Value(std::string value);
    explicit Value(bool value);

    [[nodiscard]] Type type() const noexcept;
    [[nodiscard]] std::int32_t as_int() const;
    [[nodiscard]] const std::string& as_varchar() const;
    [[nodiscard]] bool as_boolean() const;

    [[nodiscard]] bool operator==(const Value& other) const noexcept;

private:
    Type type_;
    std::variant<std::int32_t, std::string, bool> data_;
};

}  // namespace minidb
