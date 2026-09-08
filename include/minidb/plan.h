#pragma once

#include "minidb/ast.h"

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace minidb {

enum class PlanType { SeqScan, IndexScan, Filter, Insert, Update, Delete, CreateTable, CreateIndex };

struct SeqScanPlan {
    std::string table_name;
    std::vector<std::string> columns;
};

struct IndexScanPlan {
    std::string table_name;
    std::vector<std::string> columns;
    std::string index_name;
    std::int32_t key;
};

struct FilterPlan {
    std::unique_ptr<struct PlanNode> child;
    std::unique_ptr<Expression> predicate;
};

struct InsertPlan {
    std::string table_name;
    std::vector<Value> values;
};

struct UpdatePlan {
    std::string table_name;
    std::string column_name;
    Value value;
    std::unique_ptr<Expression> predicate;
};

struct DeletePlan {
    std::string table_name;
    std::unique_ptr<Expression> predicate;
};

struct CreateTablePlan {
    std::string table_name;
    Schema schema;
};

struct CreateIndexPlan {
    std::string index_name;
    std::string table_name;
    std::string column_name;
};

using PlanDetails = std::variant<SeqScanPlan, IndexScanPlan, FilterPlan, InsertPlan, UpdatePlan,
                                 DeletePlan, CreateTablePlan, CreateIndexPlan>;

struct PlanNode {
    PlanType type;
    PlanDetails details;
};

}  // namespace minidb
