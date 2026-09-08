#include "minidb/parser.h"
#include "minidb/planner.h"

#include <cassert>
#include <string>

using namespace std;

int main() {
    const minidb::Statement select = minidb::Parser::parse_sql(
        "SELECT name FROM students WHERE id = 1;");
    const minidb::PlanNode select_plan = minidb::Planner::make_plan(select);
    assert(select_plan.type == minidb::PlanType::Filter);
    const auto& filter = get<minidb::FilterPlan>(select_plan.details);
    assert(filter.child->type == minidb::PlanType::SeqScan);
    assert(get<minidb::SeqScanPlan>(filter.child->details).table_name == "students");
    assert(filter.predicate != nullptr);

    const minidb::Statement plain_select = minidb::Parser::parse_sql("SELECT * FROM students;");
    const minidb::PlanNode scan_plan = minidb::Planner::make_plan(plain_select);
    assert(scan_plan.type == minidb::PlanType::SeqScan);

    const minidb::Statement insert = minidb::Parser::parse_sql(
        "INSERT INTO students VALUES (1, 'Lakshya');");
    const minidb::PlanNode insert_plan = minidb::Planner::make_plan(insert);
    assert(insert_plan.type == minidb::PlanType::Insert);
    assert(get<minidb::InsertPlan>(insert_plan.details).values.size() == 2);

    const minidb::Statement create = minidb::Parser::parse_sql(
        "CREATE TABLE students (id INT, name VARCHAR);");
    const minidb::PlanNode create_plan = minidb::Planner::make_plan(create);
    assert(create_plan.type == minidb::PlanType::CreateTable);
    assert(get<minidb::CreateTablePlan>(create_plan.details).schema.column_count() == 2);

    const minidb::Statement update = minidb::Parser::parse_sql(
        "UPDATE students SET name = 'New' WHERE id = 1;");
    assert(minidb::Planner::make_plan(update).type == minidb::PlanType::Update);

    const minidb::Statement delete_statement = minidb::Parser::parse_sql(
        "DELETE FROM students WHERE id = 1;");
    assert(minidb::Planner::make_plan(delete_statement).type == minidb::PlanType::Delete);

    const minidb::Statement create_index = minidb::Parser::parse_sql(
        "CREATE INDEX idx_id ON students(id);");
    const minidb::PlanNode index_plan = minidb::Planner::make_plan(create_index);
    assert(index_plan.type == minidb::PlanType::CreateIndex);
    assert(get<minidb::CreateIndexPlan>(index_plan.details).column_name == "id");
    return 0;
}
