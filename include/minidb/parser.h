#pragma once

#include "minidb/ast.h"
#include "minidb/token.h"

#include <string>
#include <vector>

namespace minidb {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    [[nodiscard]] Statement parse_statement();
    [[nodiscard]] static Statement parse_sql(const std::string& sql);

private:
    [[nodiscard]] const Token& current() const;
    [[nodiscard]] bool check(TokenType type) const noexcept;
    [[nodiscard]] bool match(TokenType type) noexcept;
    const Token& consume(TokenType type, const std::string& expectation);
    [[noreturn]] void error(const std::string& message) const;

    [[nodiscard]] Statement parse_create();
    [[nodiscard]] Statement parse_insert();
    [[nodiscard]] Statement parse_select();
    [[nodiscard]] Statement parse_update();
    [[nodiscard]] Statement parse_delete();
    [[nodiscard]] std::unique_ptr<Expression> parse_expression();
    [[nodiscard]] std::unique_ptr<Expression> parse_or();
    [[nodiscard]] std::unique_ptr<Expression> parse_and();
    [[nodiscard]] std::unique_ptr<Expression> parse_comparison();
    [[nodiscard]] std::unique_ptr<Expression> parse_primary();
    [[nodiscard]] Value parse_literal();
    [[nodiscard]] Type parse_type();
    [[nodiscard]] ComparisonOperator parse_comparison_operator();

    std::vector<Token> tokens_;
    std::size_t position_{0};
};

}  // namespace minidb
