#include "minidb/tuple.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace minidb {

namespace {

void append_uint32(std::vector<std::byte>& bytes, std::uint32_t value) {
    for (std::size_t shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffU));
    }
}

std::uint32_t read_uint32(const std::vector<std::byte>& bytes, std::size_t& offset) {
    if (bytes.size() - offset < sizeof(std::uint32_t)) {
        throw std::invalid_argument("Tuple data is truncated");
    }

    std::uint32_t value = 0;
    for (std::size_t shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[offset++])) << shift;
    }
    return value;
}

void validate_value_type(const Value& value, Type expected) {
    if (value.type() != expected) {
        throw std::invalid_argument("Tuple value type does not match schema");
    }
}

}  // namespace

Tuple::Tuple(std::vector<Value> values) : values_(std::move(values)) {}

std::size_t Tuple::size() const noexcept {
    return values_.size();
}

const Value& Tuple::value_at(std::size_t index) const {
    if (index >= values_.size()) {
        throw std::out_of_range("Tuple value index is out of range");
    }
    return values_[index];
}

const std::vector<Value>& Tuple::values() const noexcept {
    return values_;
}

std::vector<std::byte> Tuple::serialize(const Schema& schema) const {
    if (values_.size() != schema.column_count()) {
        throw std::invalid_argument("Tuple value count does not match schema");
    }

    std::vector<std::byte> bytes;
    for (std::size_t index = 0; index < values_.size(); ++index) {
        const Value& value = values_[index];
        const Type expected_type = schema.column(index).type();
        validate_value_type(value, expected_type);
        bytes.push_back(static_cast<std::byte>(value.type()));

        switch (value.type()) {
            case Type::Int:
                append_uint32(bytes, static_cast<std::uint32_t>(value.as_int()));
                break;
            case Type::Varchar: {
                const std::string& text = value.as_varchar();
                if (text.size() > std::numeric_limits<std::uint32_t>::max()) {
                    throw std::length_error("VARCHAR value is too large to serialize");
                }
                append_uint32(bytes, static_cast<std::uint32_t>(text.size()));
                for (const char character : text) {
                    bytes.push_back(static_cast<std::byte>(character));
                }
                break;
            }
            case Type::Boolean:
                bytes.push_back(static_cast<std::byte>(value.as_boolean() ? 1 : 0));
                break;
        }
    }
    return bytes;
}

Tuple Tuple::deserialize(const Schema& schema, const std::vector<std::byte>& bytes) {
    std::vector<Value> values;
    values.reserve(schema.column_count());
    std::size_t offset = 0;

    for (std::size_t index = 0; index < schema.column_count(); ++index) {
        if (offset >= bytes.size()) {
            throw std::invalid_argument("Tuple data is truncated");
        }

        const auto encoded_type = static_cast<Type>(std::to_integer<unsigned char>(bytes[offset++]));
        const Type expected_type = schema.column(index).type();
        if (encoded_type != expected_type) {
            throw std::invalid_argument("Serialized tuple type does not match schema");
        }

        switch (encoded_type) {
            case Type::Int:
                values.emplace_back(static_cast<std::int32_t>(read_uint32(bytes, offset)));
                break;
            case Type::Varchar: {
                const std::uint32_t length = read_uint32(bytes, offset);
                if (bytes.size() - offset < length) {
                    throw std::invalid_argument("Tuple VARCHAR data is truncated");
                }
                std::string text;
                text.reserve(length);
                for (std::uint32_t character = 0; character < length; ++character) {
                    text.push_back(static_cast<char>(std::to_integer<unsigned char>(bytes[offset++])));
                }
                values.emplace_back(std::move(text));
                break;
            }
            case Type::Boolean:
                if (offset >= bytes.size()) {
                    throw std::invalid_argument("Tuple BOOLEAN data is truncated");
                }
                if (bytes[offset] != std::byte{0} && bytes[offset] != std::byte{1}) {
                    throw std::invalid_argument("Serialized BOOLEAN value is invalid");
                }
                values.emplace_back(bytes[offset++] == std::byte{1});
                break;
            default:
                throw std::invalid_argument("Serialized tuple type is invalid");
        }
    }

    if (offset != bytes.size()) {
        throw std::invalid_argument("Serialized tuple contains trailing data");
    }
    return Tuple(std::move(values));
}

}  // namespace minidb
