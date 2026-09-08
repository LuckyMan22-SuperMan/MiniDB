#include "minidb/parser.h"

#include "minidb/lexer.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace minidb {

using namespace std;

Parser::Parser(vector<Token> tokens) : tokens_(std::move(tokens)) {
    if (tokens_.empty() || tokens_.back().type != TokenType::EndOfInput) {
        throw invalid_argument("Parser input must end with an end-of-input token");
    }
}

Statement Parser::parse_sql(const string& sql) {
    return Parser(Lexer(sql).tokenize()).parse_statement();
}

Statement Parser::parse_statement() {
    Statement statement = [&]() -> Statement {
        switch (current().type) {
            case TokenType::Create: return parse_create();
            case TokenType::Insert: return parse_insert();
            case TokenType::Select: return parse_select();
            case TokenType::Update: return parse_update();
            case TokenType::Delete: return parse_delete();
            default: error("Expected a SQL statement");
        }
    }();

    (void)match(TokenType::Semicolon);
    if (!check(TokenType::EndOfInput)) {
        error("Expected end of statement");
    }
    return statement;
}

const Token& Parser::current() const {
    return tokens_[position_];
}

bool Parser::check(TokenType type) const noexcept {
    return current().type == type;
}

bool Parser::match(TokenType type) noexcept {
    if (!check(type)) {
        return false;
    }
    ++position_;
    return true;
}

const Token& Parser::consume(TokenType type, const string& expectation) {
    if (!check(type)) {
        error("Expected " + expectation + ", got " + token_type_name(current().type));
    }
    return tokens_[position_++];
}

[[noreturn]] void Parser::error(const string& message) const {
    throw invalid_argument(message + " at line " + to_string(current().line) +
                            ", column " + to_string(current().column));
}

Statement Parser::parse_create() {
    consume(TokenType::Create, "CREATE");
    if (match(TokenType::Table)) {
        const string table_name = consume(TokenType::Identifier, "table name").lexeme;
        consume(TokenType::LeftParen, "'('");
        vector<ColumnDefinition> columns;
        do {
            const string column_name = consume(TokenType::Identifier, "column name").lexeme;
            columns.push_back(ColumnDefinition{column_name, parse_type()});
        } while (match(TokenType::Comma));
        consume(TokenType::RightParen, "')'");
        if (columns.empty()) error("CREATE TABLE requires at least one column");
        return CreateTableStatement{table_name, std::move(columns)};
    }
    if (match(TokenType::Index)) {
        const string index_name = consume(TokenType::Identifier, "index name").lexeme;
        consume(TokenType::On, "ON");
        const string table_name = consume(TokenType::Identifier, "table name").lexeme;
        consume(TokenType::LeftParen, "'('");
        const string column_name = consume(TokenType::Identifier, "column name").lexeme;
        consume(TokenType::RightParen, "')'");
        return CreateIndexStatement{index_name, table_name, column_name};
    }
    error("Expected TABLE or INDEX after CREATE");
}

Statement Parser::parse_insert() {
    consume(TokenType::Insert, "INSERT");
    consume(TokenType::Into, "INTO");
    const string table_name = consume(TokenType::Identifier, "table name").lexeme;
    consume(TokenType::Values, "VALUES");
    consume(TokenType::LeftParen, "'('");
    vector<Value> values;
    do {
        values.push_back(parse_literal());
    } while (match(TokenType::Comma));
    consume(TokenType::RightParen, "')'");
    return InsertStatement{table_name, std::move(values)};
}

Statement Parser::parse_select() {
    consume(TokenType::Select, "SELECT");
    vector<string> columns;
    if (match(TokenType::Star)) {
        columns.push_back("*");
    } else {
        do {
            columns.push_back(consume(TokenType::Identifier, "column name").lexeme);
        } while (match(TokenType::Comma));
    }
    consume(TokenType::From, "FROM");
    const string table_name = consume(TokenType::Identifier, "table name").lexeme;
    unique_ptr<Expression> where;
    if (match(TokenType::Where)) {
        where = parse_expression();
    }
    return SelectStatement{std::move(columns), table_name, std::move(where)};
}

Statement Parser::parse_update() {
    consume(TokenType::Update, "UPDATE");
    const string table_name = consume(TokenType::Identifier, "table name").lexeme;
    consume(TokenType::Set, "SET");
    const string column_name = consume(TokenType::Identifier, "column name").lexeme;
    consume(TokenType::Equal, "'='");
    Value value = parse_literal();
    unique_ptr<Expression> where;
    if (match(TokenType::Where)) {
        where = parse_expression();
    }
    return UpdateStatement{table_name, column_name, std::move(value), std::move(where)};
}

Statement Parser::parse_delete() {
    consume(TokenType::Delete, "DELETE");
    consume(TokenType::From, "FROM");
    const string table_name = consume(TokenType::Identifier, "table name").lexeme;
    unique_ptr<Expression> where;
    if (match(TokenType::Where)) {
        where = parse_expression();
    }
    return DeleteStatement{table_name, std::move(where)};
}

unique_ptr<Expression> Parser::parse_expression() {
    return parse_or();
}

unique_ptr<Expression> Parser::parse_or() {
    unique_ptr<Expression> expression = parse_and();
    while (match(TokenType::Or)) {
        auto right = parse_and();
        expression = make_unique<Expression>(LogicalExpression{std::move(expression), LogicalOperator::Or,
                                       std::move(right)});
    }
    return expression;
}

unique_ptr<Expression> Parser::parse_and() {
    unique_ptr<Expression> expression = parse_comparison();
    while (match(TokenType::And)) {
        auto right = parse_comparison();
        expression = make_unique<Expression>(LogicalExpression{std::move(expression), LogicalOperator::And,
                                       std::move(right)});
    }
    return expression;
}

unique_ptr<Expression> Parser::parse_comparison() {
    auto left = parse_primary();
    if (!check(TokenType::Equal) && !check(TokenType::NotEqual) && !check(TokenType::Less) &&
        !check(TokenType::LessEqual) && !check(TokenType::Greater) && !check(TokenType::GreaterEqual)) {
        return left;
    }
    const ComparisonOperator op = parse_comparison_operator();
    auto right = parse_primary();
    return make_unique<Expression>(ComparisonExpression{std::move(left), op, std::move(right)});
}

unique_ptr<Expression> Parser::parse_primary() {
    if (match(TokenType::LeftParen)) {
        auto expression = parse_expression();
        consume(TokenType::RightParen, "')'");
        return expression;
    }
    if (check(TokenType::Identifier)) {
        return make_unique<Expression>(ColumnReference{consume(TokenType::Identifier, "column name").lexeme});
    }
    if (check(TokenType::IntegerLiteral) || check(TokenType::StringLiteral) || check(TokenType::True) ||
        check(TokenType::False)) {
        return make_unique<Expression>(LiteralExpression{parse_literal()});
    }
    error("Expected a column reference or literal");
}

Value Parser::parse_literal() {
    if (check(TokenType::IntegerLiteral)) {
        return Value(static_cast<int32_t>(stoi(consume(TokenType::IntegerLiteral, "integer literal").lexeme)));
    }
    if (check(TokenType::StringLiteral)) {
        return Value(consume(TokenType::StringLiteral, "string literal").lexeme);
    }
    if (match(TokenType::True)) return Value(true);
    if (match(TokenType::False)) return Value(false);
    error("Expected a literal value");
}

Type Parser::parse_type() {
    if (match(TokenType::Int)) return Type::Int;
    if (match(TokenType::Varchar)) return Type::Varchar;
    if (match(TokenType::Boolean)) return Type::Boolean;
    error("Expected INT, VARCHAR, or BOOLEAN");
}

ComparisonOperator Parser::parse_comparison_operator() {
    if (match(TokenType::Equal)) return ComparisonOperator::Equal;
    if (match(TokenType::NotEqual)) return ComparisonOperator::NotEqual;
    if (match(TokenType::Less)) return ComparisonOperator::Less;
    if (match(TokenType::LessEqual)) return ComparisonOperator::LessEqual;
    if (match(TokenType::Greater)) return ComparisonOperator::Greater;
    if (match(TokenType::GreaterEqual)) return ComparisonOperator::GreaterEqual;
    error("Expected a comparison operator");
}

}  // namespace minidb
