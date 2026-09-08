#include "minidb/execution.h"

#include "minidb/planner.h"

#include <stdexcept>
#include <utility>

namespace minidb {

using namespace std;

ExecutionEngine::ExecutionEngine(BufferPoolManager& buffer_pool, Catalog& catalog)
    : buffer_pool_(buffer_pool), catalog_(catalog) {
    for (const string& name : catalog_.table_names()) {
        const optional<TableMetadata> metadata = catalog_.get_table(name);
        if (metadata.has_value()) {
            tables_.emplace(name, make_unique<TableHeap>(buffer_pool_, metadata->schema,
                                                         metadata->page_ids));
        }
    }
}

QueryResult ExecutionEngine::execute(const Statement& statement) {
    const PlanNode plan = Planner::make_plan(statement);
    switch (plan.type) {
        case PlanType::CreateTable: {
            const auto& create = get<CreateTablePlan>(plan.details);
            const uint32_t table_id = catalog_.create_table(create.table_name, create.schema);
            (void)table_id;
            tables_.emplace(create.table_name,
                            make_unique<TableHeap>(buffer_pool_, create.schema));
            return QueryResult{ {}, {}, 0 };
        }
        case PlanType::Insert: {
            const auto& insert = get<InsertPlan>(plan.details);
            TableHeap& heap = table(insert.table_name);
            const optional<RID> rid = heap.insert_tuple(Tuple(insert.values));
            if (!rid.has_value()) {
                throw runtime_error("Unable to insert tuple");
            }
            const bool updated_catalog = catalog_.update_table_pages(insert.table_name,
                                                                       heap.page_ids());
            (void)updated_catalog;
            return QueryResult{ {}, {}, 1 };
        }
        case PlanType::SeqScan: {
            return execute_select(get<SeqScanPlan>(plan.details), nullptr);
        }
        case PlanType::Filter: {
            const auto& filter = get<FilterPlan>(plan.details);
            const auto& scan = get<SeqScanPlan>(filter.child->details);
            return execute_select(scan, filter.predicate.get());
        }
        case PlanType::Update:
        case PlanType::Delete:
        case PlanType::CreateIndex:
            throw logic_error("This plan type is not executable in Phase 9");
    }
    throw logic_error("Unknown plan type");
}

TableHeap& ExecutionEngine::table(const string& name) {
    const auto found = tables_.find(name);
    if (found == tables_.end()) {
        throw invalid_argument("Unknown table: " + name);
    }
    return *found->second;
}

QueryResult ExecutionEngine::execute_select(const SeqScanPlan& scan,
                                             const Expression* predicate) {
    TableHeap& heap = table(scan.table_name);
    QueryResult result;
    if (scan.columns.size() == 1 && scan.columns.front() == "*") {
        for (size_t index = 0; index < heap.schema().column_count(); ++index) {
            result.columns.push_back(heap.schema().column(index).name());
        }
    } else {
        for (const string& column_name : scan.columns) {
            result.columns.push_back(heap.schema().column(heap.schema().column_index(column_name)).name());
        }
    }

    for (const Tuple& tuple : heap.scan()) {
        if (predicate != nullptr && !evaluate_predicate(*predicate, tuple, heap.schema())) {
            continue;
        }
        vector<Value> row;
        if (scan.columns.size() == 1 && scan.columns.front() == "*") {
            row = tuple.values();
        } else {
            for (const string& column_name : scan.columns) {
                row.push_back(tuple.value_at(heap.schema().column_index(column_name)));
            }
        }
        result.rows.push_back(std::move(row));
    }
    return result;
}

bool ExecutionEngine::evaluate_predicate(const Expression& expression,
                                          const Tuple& tuple, const Schema& schema) const {
    return visit([&](const auto& node) -> bool {
        using NodeType = decay_t<decltype(node)>;
        if constexpr (is_same_v<NodeType, LogicalExpression>) {
            const bool left = evaluate_predicate(*node.left, tuple, schema);
            const bool right = evaluate_predicate(*node.right, tuple, schema);
            return node.op == LogicalOperator::And ? left && right : left || right;
        } else if constexpr (is_same_v<NodeType, ComparisonExpression>) {
            return compare_values(evaluate_value(*node.left, tuple, schema), node.op,
                                  evaluate_value(*node.right, tuple, schema));
        } else {
            return evaluate_value(expression, tuple, schema).as_boolean();
        }
    }, expression.node);
}

Value ExecutionEngine::evaluate_value(const Expression& expression,
                                       const Tuple& tuple, const Schema& schema) const {
    return visit([&](const auto& node) -> Value {
        using NodeType = decay_t<decltype(node)>;
        if constexpr (is_same_v<NodeType, ColumnReference>) {
            return tuple.value_at(schema.column_index(node.name));
        } else if constexpr (is_same_v<NodeType, LiteralExpression>) {
            return node.value;
        } else {
            throw invalid_argument("Expected a scalar expression");
        }
    }, expression.node);
}

bool ExecutionEngine::compare_values(const Value& left, ComparisonOperator op,
                                      const Value& right) const {
    if (left.type() != right.type()) {
        throw invalid_argument("Cannot compare values of different types");
    }

    int comparison = 0;
    switch (left.type()) {
        case Type::Int:
            comparison = left.as_int() < right.as_int() ? -1 : left.as_int() > right.as_int() ? 1 : 0;
            break;
        case Type::Varchar:
            comparison = left.as_varchar() < right.as_varchar() ? -1 :
                         left.as_varchar() > right.as_varchar() ? 1 : 0;
            break;
        case Type::Boolean:
            comparison = static_cast<int>(left.as_boolean()) - static_cast<int>(right.as_boolean());
            break;
    }

    switch (op) {
        case ComparisonOperator::Equal: return comparison == 0;
        case ComparisonOperator::NotEqual: return comparison != 0;
        case ComparisonOperator::Less: return comparison < 0;
        case ComparisonOperator::LessEqual: return comparison <= 0;
        case ComparisonOperator::Greater: return comparison > 0;
        case ComparisonOperator::GreaterEqual: return comparison >= 0;
    }
    return false;
}

}  // namespace minidb
