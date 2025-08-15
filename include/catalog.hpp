#pragma once
#include <string>
#include <vector>
#include <optional>

// Reuse your DataType enum and getDataType(string) from DataType.hpp
#include "DataType.hpp"

// Column definition with constraints
struct ColumnDef {
    std::string name;
    DataType type;
    bool isPrimaryKey = false;
    bool notNull = false;
    // Foreign key (optional)
    bool hasForeignKey = false;
    std::string fkTable;
    std::string fkColumn;
};

struct TableDef {
    std::string name;
    std::vector<ColumnDef> columns;
};

class Catalog {
public:
    // Load the whole catalog from metadata/catalog.meta (creates empty if missing)
    static Catalog load();

    // Save entire catalog back to disk
    bool save() const;

    // CRUD on table metadata
    bool addTable(const TableDef& tdef);        // returns false if table exists
    std::optional<TableDef> getTable(const std::string& name) const;
    bool tableExists(const std::string& name) const;
    std::vector<std::string> listTables() const;

private:
    std::vector<TableDef> tables;

    // parsing / serialization
    static std::string catalogPath();
    static void ensureMetadataDir();

    static std::string serialize(const Catalog& c);
    static Catalog parse(const std::string& text);
};
