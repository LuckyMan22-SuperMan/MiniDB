#pragma once

#include "minidb/buffer_pool_manager.h"
#include "minidb/catalog.h"
#include "minidb/index_manager.h"
#include "minidb/plan.h"
#include "minidb/table_heap.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minidb {

struct QueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<Value>> rows;
    std::size_t affected_rows{0};
};

class ExecutionEngine {
public:
    ExecutionEngine(BufferPoolManager& buffer_pool, Catalog& catalog);

    [[nodiscard]] QueryResult execute(const Statement& statement);

private:
    [[nodiscard]] TableHeap& table(const std::string& name);
    [[nodiscard]] QueryResult execute_select(const SeqScanPlan& scan,
                                              const Expression* predicate);
    [[nodiscard]] QueryResult execute_update(const UpdatePlan& update);
    [[nodiscard]] QueryResult execute_delete(const DeletePlan& delete_plan);
    [[nodiscard]] bool evaluate_predicate(const Expression& expression,
                                          const Tuple& tuple, const Schema& schema) const;
    [[nodiscard]] Value evaluate_value(const Expression& expression,
                                       const Tuple& tuple, const Schema& schema) const;
    [[nodiscard]] bool compare_values(const Value& left, ComparisonOperator op,
                                      const Value& right) const;

    BufferPoolManager& buffer_pool_;
    Catalog& catalog_;
    std::unique_ptr<IndexManager> index_manager_;
    std::unordered_map<std::string, std::unique_ptr<TableHeap>> tables_;
};

}  // namespace minidb
