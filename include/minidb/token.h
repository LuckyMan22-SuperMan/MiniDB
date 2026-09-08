#pragma once

#include <cstddef>
#include <string>

namespace minidb {

enum class TokenType {
    EndOfInput,
    Identifier,
    IntegerLiteral,
    StringLiteral,
    Create,
    Table,
    Insert,
    Into,
    Values,
    Select,
    From,
    Where,
    Update,
    Set,
    Delete,
    Index,
    On,
    Int,
    Varchar,
    Boolean,
    True,
    False,
    And,
    Or,
    Star,
    Comma,
    LeftParen,
    RightParen,
    Semicolon,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Plus,
    Minus,
};

struct Token {
    TokenType type;
    std::string lexeme;
    std::size_t line;
    std::size_t column;
};

[[nodiscard]] const char* token_type_name(TokenType type) noexcept;

}  // namespace minidb
