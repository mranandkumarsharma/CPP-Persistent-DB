#pragma once
#include "Column.hpp"
#include <vector>
#include <string>

class Schema {
public:
    Schema() = default;
    Schema(const std::vector<Column>& cols);

    void addColumn(const Column& col);
    const std::vector<Column>& getColumns() const;

    bool saveToFile(const std::string& tableName) const;
    static Schema loadFromFile(const std::string& tableName);

private:
    std::vector<Column> columns;
};
