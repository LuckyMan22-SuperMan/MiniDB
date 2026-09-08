#include "minidb/lexer.h"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

int main() {
    const string sql =
        "CREATE TABLE students (id INT, name VARCHAR, active BOOLEAN);\n"
        "-- filter students\n"
        "SELECT name FROM students WHERE id >= 10 AND active = TRUE;";
    const vector<minidb::Token> tokens = minidb::Lexer(sql).tokenize();

    assert(tokens[0].type == minidb::TokenType::Create);
    assert(tokens[1].type == minidb::TokenType::Table);
    assert(tokens[2].type == minidb::TokenType::Identifier);
    assert(tokens[2].lexeme == "students");
    assert(tokens[4].type == minidb::TokenType::Identifier);
    assert(tokens[5].type == minidb::TokenType::Int);
    assert(tokens[8].type == minidb::TokenType::Varchar);
    assert(tokens[11].type == minidb::TokenType::Boolean);
    assert(tokens[13].type == minidb::TokenType::Semicolon);
    assert(tokens[14].type == minidb::TokenType::Select);
    assert(tokens[20].type == minidb::TokenType::GreaterEqual);
    assert(tokens[22].type == minidb::TokenType::And);
    assert(tokens[24].type == minidb::TokenType::Equal);
    assert(tokens[25].type == minidb::TokenType::True);
    assert(tokens.back().type == minidb::TokenType::EndOfInput);

    const vector<minidb::Token> literals = minidb::Lexer("INSERT INTO t VALUES (1, 'Lakshya''s');").tokenize();
    assert(literals[4].type == minidb::TokenType::LeftParen);
    assert(literals[5].type == minidb::TokenType::IntegerLiteral);
    assert(literals[7].type == minidb::TokenType::StringLiteral);
    assert(literals[7].lexeme == "Lakshya's");

    bool rejected_character = false;
    try {
        const auto ignored = minidb::Lexer("SELECT @;").tokenize();
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_character = true;
    }
    assert(rejected_character);

    bool rejected_string = false;
    try {
        const auto ignored = minidb::Lexer("SELECT 'unterminated").tokenize();
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_string = true;
    }
    assert(rejected_string);

    bool rejected_integer = false;
    try {
        const auto ignored = minidb::Lexer("SELECT 2147483648;").tokenize();
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_integer = true;
    }
    assert(rejected_integer);
    return 0;
}
