#include "minidb/value.h"

#include <stdexcept>
#include <utility>

namespace minidb {

Value::Value(std::int32_t value) : type_(Type::Int), data_(value) {}

Value::Value(std::string value) : type_(Type::Varchar), data_(std::move(value)) {}

Value::Value(bool value) : type_(Type::Boolean), data_(value) {}

Type Value::type() const noexcept {
    return type_;
}

std::int32_t Value::as_int() const {
    if (type_ != Type::Int) {
        throw std::logic_error("Value is not an INT");
    }
    return std::get<std::int32_t>(data_);
}

const std::string& Value::as_varchar() const {
    if (type_ != Type::Varchar) {
        throw std::logic_error("Value is not a VARCHAR");
    }
    return std::get<std::string>(data_);
}

bool Value::as_boolean() const {
    if (type_ != Type::Boolean) {
        throw std::logic_error("Value is not a BOOLEAN");
    }
    return std::get<bool>(data_);
}

bool Value::operator==(const Value& other) const noexcept {
    return type_ == other.type_ && data_ == other.data_;
}

}  // namespace minidb
