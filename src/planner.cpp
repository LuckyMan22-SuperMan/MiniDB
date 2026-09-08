#include "minidb/planner.h"

#include <stdexcept>
#include <utility>

namespace minidb {

using namespace std;

PlanNode Planner::make_plan(const Statement& statement, const Catalog* catalog) {
    return visit([catalog](const auto& typed_statement) -> PlanNode {
        using StatementType = decay_t<decltype(typed_statement)>;
        if constexpr (is_same_v<StatementType, CreateTableStatement>) {
            return plan_create_table(typed_statement);
        } else if constexpr (is_same_v<StatementType, InsertStatement>) {
            return plan_insert(typed_statement);
        } else if constexpr (is_same_v<StatementType, SelectStatement>) {
            return plan_select(typed_statement, catalog);
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

PlanNode Planner::plan_select(const SelectStatement& statement, const Catalog* catalog) {
    if (catalog != nullptr && statement.where != nullptr) {
        const auto* comparison = get_if<ComparisonExpression>(&statement.where->node);
        if (comparison != nullptr && comparison->op == ComparisonOperator::Equal) {
            const auto* column = get_if<ColumnReference>(&comparison->left->node);
            const auto* literal = get_if<LiteralExpression>(&comparison->right->node);
            if (column == nullptr || literal == nullptr) {
                column = get_if<ColumnReference>(&comparison->right->node);
                literal = get_if<LiteralExpression>(&comparison->left->node);
            }
            if (column != nullptr && literal != nullptr && literal->value.type() == Type::Int) {
                for (const IndexMetadata& index : catalog->indexes_for_table(statement.table_name)) {
                    if (index.column_name == column->name) {
                        return PlanNode{PlanType::IndexScan,
                                        IndexScanPlan{statement.table_name, statement.columns,
                                                      index.name, literal->value.as_int()}};
                    }
                }
            }
        }
    }
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
