#pragma once

#include "minidb/token.h"

#include <string>
#include <vector>

namespace minidb {

class Lexer {
public:
    explicit Lexer(std::string input);

    [[nodiscard]] Token next();
    [[nodiscard]] std::vector<Token> tokenize();

private:
    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] char current() const noexcept;
    char advance() noexcept;
    [[nodiscard]] bool match(char expected) noexcept;
    void skip_ignored();
    [[nodiscard]] Token identifier_or_keyword();
    [[nodiscard]] Token number();
    [[nodiscard]] Token string_literal();
    [[nodiscard]] Token make_token(TokenType type, std::size_t start,
                                    std::size_t line, std::size_t column) const;
    [[noreturn]] void error(const std::string& message) const;

    std::string input_;
    std::size_t position_{0};
    std::size_t line_{1};
    std::size_t column_{1};
};

}  // namespace minidb
