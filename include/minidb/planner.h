#pragma once

#include "minidb/ast.h"
#include "minidb/plan.h"

namespace minidb {

class Planner {
public:
    [[nodiscard]] static PlanNode make_plan(const Statement& statement);

private:
    [[nodiscard]] static PlanNode plan_create_table(const CreateTableStatement& statement);
    [[nodiscard]] static PlanNode plan_insert(const InsertStatement& statement);
    [[nodiscard]] static PlanNode plan_select(const SelectStatement& statement);
    [[nodiscard]] static PlanNode plan_update(const UpdateStatement& statement);
    [[nodiscard]] static PlanNode plan_delete(const DeleteStatement& statement);
    [[nodiscard]] static PlanNode plan_create_index(const CreateIndexStatement& statement);
    [[nodiscard]] static std::unique_ptr<Expression> clone_expression(const Expression& expression);
};

}  // namespace minidb
