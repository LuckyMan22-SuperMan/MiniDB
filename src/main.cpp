#include "minidb/catalog.h"
#include "minidb/execution.h"
#include "minidb/parser.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <variant>

using namespace std;

namespace {

string value_to_string(const minidb::Value& value) {
    switch (value.type()) {
        case minidb::Type::Int: return to_string(value.as_int());
        case minidb::Type::Varchar: return value.as_varchar();
        case minidb::Type::Boolean: return value.as_boolean() ? "TRUE" : "FALSE";
    }
    return {};
}

void print_result(const minidb::QueryResult& result) {
    if (result.columns.empty()) {
        cout << result.affected_rows << " row(s) affected." << '\n';
        return;
    }

    for (const string& column : result.columns) cout << "| " << column << ' ';
    cout << '|' << '\n';
    for (const auto& row : result.rows) {
        for (const minidb::Value& value : row) cout << "| " << value_to_string(value) << ' ';
        cout << '|' << '\n';
    }
    cout << result.rows.size() << " row(s)." << '\n';
}

bool handle_dot_command(const string& input, const minidb::Catalog& catalog) {
    if (input == ".help") {
        cout << ".help  .tables  .schema [table]  .exit" << '\n';
        return true;
    }
    if (input == ".tables") {
        for (const string& table : catalog.table_names()) cout << table << '\n';
        return true;
    }
    if (input.rfind(".schema", 0) == 0) {
        const string table_name = input.size() > 7 ? input.substr(8) : string{};
        if (table_name.empty()) {
            cout << "Usage: .schema <table>" << '\n';
            return true;
        }
        const auto metadata = catalog.get_table(table_name);
        if (!metadata.has_value()) {
            cout << "Unknown table: " << table_name << '\n';
            return true;
        }
        for (size_t index = 0; index < metadata->schema.column_count(); ++index) {
            cout << metadata->schema.column(index).name() << '\n';
        }
        return true;
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    const filesystem::path database_path = argc > 1 ? argv[1] : "minidb.db";
    const filesystem::path catalog_path = database_path.string() + ".catalog";
    minidb::DiskManager disk_manager(database_path);
    minidb::BufferPoolManager buffer_pool(disk_manager, 32);
    minidb::Catalog catalog(catalog_path);
    minidb::ExecutionEngine engine(buffer_pool, catalog);

    cout << "MiniDB v0.1" << '\n';
    cout << "Type .help for help." << '\n';
    string input;
    while (cout << "minidb> " && getline(cin, input)) {
        if (input.empty()) continue;
        if (input == ".exit") break;
        if (input[0] == '.') {
            if (!handle_dot_command(input, catalog)) cout << "Unknown command." << '\n';
            continue;
        }
        try {
            const minidb::Statement statement = minidb::Parser::parse_sql(input);
            const minidb::QueryResult result = engine.execute(statement);
            visit([&](const auto& typed_statement) {
                using StatementType = decay_t<decltype(typed_statement)>;
                if constexpr (is_same_v<StatementType, minidb::CreateTableStatement>) {
                    cout << "Table created." << '\n';
                } else if constexpr (is_same_v<StatementType, minidb::CreateIndexStatement>) {
                    cout << "Index created." << '\n';
                } else {
                    print_result(result);
                }
            }, statement);
        } catch (const exception& error) {
            cout << "Error: " << error.what() << '\n';
        }
    }
    buffer_pool.flush_all_pages();
    return 0;
}
