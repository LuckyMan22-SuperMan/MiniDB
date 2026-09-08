#include "minidb/planner.h"

#include <stdexcept>
#include <utility>

namespace minidb {

using namespace std;

PlanNode Planner::make_plan(const Statement& statement) {
    return visit([](const auto& typed_statement) -> PlanNode {
        using StatementType = decay_t<decltype(typed_statement)>;
        if constexpr (is_same_v<StatementType, CreateTableStatement>) {
            return plan_create_table(typed_statement);
        } else if constexpr (is_same_v<StatementType, InsertStatement>) {
            return plan_insert(typed_statement);
        } else if constexpr (is_same_v<StatementType, SelectStatement>) {
            return plan_select(typed_statement);
        } else if constexpr (is_same_v<StatementType, UpdateStatement>) {
            return plan_update(typed_statement);
        } else if constexpr (is_same_v<StatementType, DeleteStatement>) {
            return plan_delete(typed_statement);
        } else {
            return plan_create_index(typed_statement);
        }
    }, statement);
}

PlanNode Planner::plan_create_table(const CreateTableStatement& statement) {
    vector<Column> columns;
    columns.reserve(statement.columns.size());
    for (const ColumnDefinition& column : statement.columns) {
        columns.emplace_back(column.name, column.type);
    }
    return PlanNode{PlanType::CreateTable,
                    CreateTablePlan{statement.table_name, Schema(std::move(columns))}};
}

PlanNode Planner::plan_insert(const InsertStatement& statement) {
    return PlanNode{PlanType::Insert, InsertPlan{statement.table_name, statement.values}};
}

PlanNode Planner::plan_select(const SelectStatement& statement) {
    PlanNode scan{PlanType::SeqScan, SeqScanPlan{statement.table_name, statement.columns}};
    if (statement.where == nullptr) {
        return scan;
    }
    auto child = make_unique<PlanNode>(std::move(scan));
    return PlanNode{PlanType::Filter,
                    FilterPlan{std::move(child), clone_expression(*statement.where)}};
}

PlanNode Planner::plan_update(const UpdateStatement& statement) {
    return PlanNode{PlanType::Update,
                    UpdatePlan{statement.table_name, statement.column_name, statement.value,
                               statement.where == nullptr ? nullptr : clone_expression(*statement.where)}};
}

PlanNode Planner::plan_delete(const DeleteStatement& statement) {
    return PlanNode{PlanType::Delete,
                    DeletePlan{statement.table_name,
                               statement.where == nullptr ? nullptr : clone_expression(*statement.where)}};
}

PlanNode Planner::plan_create_index(const CreateIndexStatement& statement) {
    return PlanNode{PlanType::CreateIndex,
                    CreateIndexPlan{statement.index_name, statement.table_name,
                                    statement.column_name}};
}

unique_ptr<Expression> Planner::clone_expression(const Expression& expression) {
    return visit([](const auto& node) -> unique_ptr<Expression> {
        using NodeType = decay_t<decltype(node)>;
        if constexpr (is_same_v<NodeType, ColumnReference>) {
            return make_unique<Expression>(ColumnReference{node.name});
        } else if constexpr (is_same_v<NodeType, LiteralExpression>) {
            return make_unique<Expression>(LiteralExpression{node.value});
        } else if constexpr (is_same_v<NodeType, ComparisonExpression>) {
            return make_unique<Expression>(ComparisonExpression{
                clone_expression(*node.left), node.op, clone_expression(*node.right)});
        } else {
            return make_unique<Expression>(LogicalExpression{
                clone_expression(*node.left), node.op, clone_expression(*node.right)});
        }
    }, expression.node);
}

}  // namespace minidb
