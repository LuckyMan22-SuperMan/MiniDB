#include "minidb/lexer.h"

#include <cctype>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace minidb {

using namespace std;

namespace {

const unordered_map<string, TokenType> kKeywords{
    {"CREATE", TokenType::Create},       {"TABLE", TokenType::Table},
    {"INSERT", TokenType::Insert},       {"INTO", TokenType::Into},
    {"VALUES", TokenType::Values},       {"SELECT", TokenType::Select},
    {"FROM", TokenType::From},           {"WHERE", TokenType::Where},
    {"UPDATE", TokenType::Update},       {"SET", TokenType::Set},
    {"DELETE", TokenType::Delete},       {"INDEX", TokenType::Index},
    {"ON", TokenType::On},               {"INT", TokenType::Int},
    {"VARCHAR", TokenType::Varchar},     {"BOOLEAN", TokenType::Boolean},
    {"TRUE", TokenType::True},           {"FALSE", TokenType::False},
    {"AND", TokenType::And},             {"OR", TokenType::Or},
};

string uppercase(string value) {
    for (char& character : value) {
        character = static_cast<char>(toupper(static_cast<unsigned char>(character)));
    }
    return value;
}

bool is_identifier_start(char character) {
    return isalpha(static_cast<unsigned char>(character)) || character == '_';
}

bool is_identifier_part(char character) {
    return isalnum(static_cast<unsigned char>(character)) || character == '_';
}

}  // namespace

const char* token_type_name(TokenType type) noexcept {
    switch (type) {
        case TokenType::EndOfInput: return "end of input";
        case TokenType::Identifier: return "identifier";
        case TokenType::IntegerLiteral: return "integer literal";
        case TokenType::StringLiteral: return "string literal";
        case TokenType::Create: return "CREATE";
        case TokenType::Table: return "TABLE";
        case TokenType::Insert: return "INSERT";
        case TokenType::Into: return "INTO";
        case TokenType::Values: return "VALUES";
        case TokenType::Select: return "SELECT";
        case TokenType::From: return "FROM";
        case TokenType::Where: return "WHERE";
        case TokenType::Update: return "UPDATE";
        case TokenType::Set: return "SET";
        case TokenType::Delete: return "DELETE";
        case TokenType::Index: return "INDEX";
        case TokenType::On: return "ON";
        case TokenType::Int: return "INT";
        case TokenType::Varchar: return "VARCHAR";
        case TokenType::Boolean: return "BOOLEAN";
        case TokenType::True: return "TRUE";
        case TokenType::False: return "FALSE";
        case TokenType::And: return "AND";
        case TokenType::Or: return "OR";
        case TokenType::Star: return "*";
        case TokenType::Comma: return ",";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::Semicolon: return ";";
        case TokenType::Equal: return "=";
        case TokenType::NotEqual: return "!=";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
    }
    return "unknown";
}

Lexer::Lexer(string input) : input_(std::move(input)) {}

Token Lexer::next() {
    skip_ignored();
    const size_t start = position_;
    const size_t start_line = line_;
    const size_t start_column = column_;

    if (at_end()) {
        return make_token(TokenType::EndOfInput, start, start_line, start_column);
    }

    const char character = advance();
    if (is_identifier_start(character)) {
        while (!at_end() && is_identifier_part(current())) {
            advance();
        }
        const string word = input_.substr(start, position_ - start);
        const auto keyword = kKeywords.find(uppercase(word));
        if (keyword != kKeywords.end()) {
            return make_token(keyword->second, start, start_line, start_column);
        }
        return make_token(TokenType::Identifier, start, start_line, start_column);
    }

    if (isdigit(static_cast<unsigned char>(character))) {
        while (!at_end() && isdigit(static_cast<unsigned char>(current()))) {
            advance();
        }
        const string digits = input_.substr(start, position_ - start);
        try {
            const unsigned long long value = stoull(digits);
            if (value > numeric_limits<int32_t>::max()) {
                error("Integer literal is out of range");
            }
        } catch (const out_of_range&) {
            error("Integer literal is out of range");
        }
        return make_token(TokenType::IntegerLiteral, start, start_line, start_column);
    }

    if (character == '\'') {
        --position_;
        --column_;
        return string_literal();
    }

    switch (character) {
        case '*': return make_token(TokenType::Star, start, start_line, start_column);
        case ',': return make_token(TokenType::Comma, start, start_line, start_column);
        case '(': return make_token(TokenType::LeftParen, start, start_line, start_column);
        case ')': return make_token(TokenType::RightParen, start, start_line, start_column);
        case ';': return make_token(TokenType::Semicolon, start, start_line, start_column);
        case '+': return make_token(TokenType::Plus, start, start_line, start_column);
        case '-': return make_token(TokenType::Minus, start, start_line, start_column);
        case '=': return make_token(TokenType::Equal, start, start_line, start_column);
        case '!':
            if (match('=')) return make_token(TokenType::NotEqual, start, start_line, start_column);
            break;
        case '<':
            if (match('=')) return make_token(TokenType::LessEqual, start, start_line, start_column);
            return make_token(TokenType::Less, start, start_line, start_column);
        case '>':
            if (match('=')) return make_token(TokenType::GreaterEqual, start, start_line, start_column);
            return make_token(TokenType::Greater, start, start_line, start_column);
    }

    error("Unexpected character");
}

vector<Token> Lexer::tokenize() {
    vector<Token> tokens;
    do {
        tokens.push_back(next());
    } while (tokens.back().type != TokenType::EndOfInput);
    return tokens;
}

bool Lexer::at_end() const noexcept {
    return position_ >= input_.size();
}

char Lexer::current() const noexcept {
    return at_end() ? '\0' : input_[position_];
}

char Lexer::advance() noexcept {
    const char character = input_[position_++];
    if (character == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return character;
}

bool Lexer::match(char expected) noexcept {
    if (at_end() || current() != expected) {
        return false;
    }
    advance();
    return true;
}

void Lexer::skip_ignored() {
    while (!at_end()) {
        if (isspace(static_cast<unsigned char>(current()))) {
            advance();
            continue;
        }
        if (current() == '-' && position_ + 1 < input_.size() && input_[position_ + 1] == '-') {
            advance();
            advance();
            while (!at_end() && current() != '\n') {
                advance();
            }
            continue;
        }
        break;
    }
}

Token Lexer::identifier_or_keyword() {
    return next();
}

Token Lexer::number() {
    return next();
}

Token Lexer::string_literal() {
    const size_t start_line = line_;
    const size_t start_column = column_;
    const size_t start = position_;
    advance();
    string value;
    while (!at_end()) {
        const char character = advance();
        if (character == '\'') {
            if (!at_end() && current() == '\'') {
                advance();
                value.push_back('\'');
                continue;
            }
            return Token{TokenType::StringLiteral, value, start_line, start_column};
        }
        if (character == '\n') {
            error("String literal cannot contain a newline");
        }
        value.push_back(character);
    }
    (void)start;
    error("Unterminated string literal");
}

Token Lexer::make_token(TokenType type, size_t start, size_t line, size_t column) const {
    return Token{type, input_.substr(start, position_ - start), line, column};
}

[[noreturn]] void Lexer::error(const string& message) const {
    throw invalid_argument(message + " at line " + to_string(line_) + ", column " +
                            to_string(column_));
}

}  // namespace minidb
