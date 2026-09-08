#pragma once

#include "minidb/schema.h"

#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace minidb {

enum class ComparisonOperator { Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual };
enum class LogicalOperator { And, Or };

struct ColumnReference {
    std::string name;
};

struct LiteralExpression {
    Value value;
};

struct Expression;

struct ComparisonExpression {
    std::unique_ptr<Expression> left;
    ComparisonOperator op;
    std::unique_ptr<Expression> right;
};

struct LogicalExpression {
    std::unique_ptr<Expression> left;
    LogicalOperator op;
    std::unique_ptr<Expression> right;
};

struct Expression {
    using Node = std::variant<ColumnReference, LiteralExpression,
                              ComparisonExpression, LogicalExpression>;
    explicit Expression(Node node_value) : node(std::move(node_value)) {}
    Node node;
};

struct ColumnDefinition {
    std::string name;
    Type type;
};

struct CreateTableStatement {
    std::string table_name;
    std::vector<ColumnDefinition> columns;
};

struct InsertStatement {
    std::string table_name;
    std::vector<Value> values;
};

struct SelectStatement {
    std::vector<std::string> columns;
    std::string table_name;
    std::unique_ptr<Expression> where;
};

struct UpdateStatement {
    std::string table_name;
    std::string column_name;
    Value value;
    std::unique_ptr<Expression> where;
};

struct DeleteStatement {
    std::string table_name;
    std::unique_ptr<Expression> where;
};

struct CreateIndexStatement {
    std::string index_name;
    std::string table_name;
    std::string column_name;
};

using Statement = std::variant<CreateTableStatement, InsertStatement, SelectStatement,
                               UpdateStatement, DeleteStatement, CreateIndexStatement>;

}  // namespace minidb
