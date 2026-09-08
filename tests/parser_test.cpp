#include "minidb/parser.h"

#include <cassert>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
    const minidb::Statement create = minidb::Parser::parse_sql(
        "CREATE TABLE students (id INT, name VARCHAR, active BOOLEAN);");
    const auto& create_table = get<minidb::CreateTableStatement>(create);
    assert(create_table.table_name == "students");
    assert(create_table.columns.size() == 3);
    assert(create_table.columns[2].type == minidb::Type::Boolean);

    const minidb::Statement insert = minidb::Parser::parse_sql(
        "INSERT INTO students VALUES (1, 'Lakshya', TRUE);");
    const auto& insert_statement = get<minidb::InsertStatement>(insert);
    assert(insert_statement.values.size() == 3);
    assert(insert_statement.values[0].as_int() == 1);
    assert(insert_statement.values[1].as_varchar() == "Lakshya");
    assert(insert_statement.values[2].as_boolean());

    const minidb::Statement select = minidb::Parser::parse_sql(
        "SELECT name, active FROM students WHERE id = 1 AND active = TRUE;");
    const auto& select_statement = get<minidb::SelectStatement>(select);
    assert(select_statement.columns.size() == 2);
    assert(select_statement.where != nullptr);
    assert(holds_alternative<minidb::LogicalExpression>(select_statement.where->node));

    const minidb::Statement update = minidb::Parser::parse_sql(
        "UPDATE students SET age = 21 WHERE id >= 1;");
    const auto& update_statement = get<minidb::UpdateStatement>(update);
    assert(update_statement.column_name == "age");
    assert(update_statement.value.as_int() == 21);

    const minidb::Statement delete_statement = minidb::Parser::parse_sql(
        "DELETE FROM students WHERE id = 1;");
    assert(get<minidb::DeleteStatement>(delete_statement).table_name == "students");

    const minidb::Statement index = minidb::Parser::parse_sql(
        "CREATE INDEX idx_students_id ON students(id);");
    const auto& index_statement = get<minidb::CreateIndexStatement>(index);
    assert(index_statement.index_name == "idx_students_id");
    assert(index_statement.column_name == "id");

    bool rejected_missing_from = false;
    try {
        const auto ignored = minidb::Parser::parse_sql("SELECT * students;");
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_missing_from = true;
    }
    assert(rejected_missing_from);

    bool rejected_extra_tokens = false;
    try {
        const auto ignored = minidb::Parser::parse_sql("SELECT * FROM students junk;");
        (void)ignored;
    } catch (const invalid_argument&) {
        rejected_extra_tokens = true;
    }
    assert(rejected_extra_tokens);
    return 0;
}
