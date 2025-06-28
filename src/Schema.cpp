#include "Schema.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include "Utility.hpp"  // For getDataType() and toString()

using json = nlohmann::json;

Schema::Schema(const std::vector<Column>& cols) : columns(cols) {}

void Schema::addColumn(const Column& col) {
    columns.push_back(col);
}

const std::vector<Column>& Schema::getColumns() const {
    return columns;
}

bool Schema::saveToFile(const std::string& tableName) const {
    std::ofstream out("metadata/" + tableName + ".meta");
    if (!out.is_open()) return false;

    json j;
    for (const auto& col : columns) {
        j["columns"].push_back({
            {"name", col.name},
            {"type", toString(col.type)}
        });
    }

    out << j.dump(4);  // Pretty JSON
    out.close();
    return true;
}

Schema Schema::loadFromFile(const std::string& tableName) {
    std::ifstream in("metadata/" + tableName + ".meta");
    if (!in.is_open()) throw std::runtime_error("Schema file not found");

    json j;
    in >> j;

    std::vector<Column> cols;
    for (const auto& item : j["columns"]) {
        Column col;
        col.name = item["name"];
        col.type = getDataType(item["type"]);
        cols.push_back(col);
    }

    return Schema(cols);
}
