#include "CommandHandler.hpp"
#include "Schema.hpp"
#include "Utility.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <optional>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif
#include <catalog.hpp>

void makeDir(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

void handleCommand(int argc, char* argv[], const std::string& command) {
    if (command == "table_banao") {
    if (argc < 4) {
        std::cout << "Usage: cdb table_banao <table> <col:type[:pk][:notnull][:fk=tbl.col]> ...\n";
        return;
    }

    std::string tableName = argv[2];

    TableDef tdef;
    tdef.name = tableName;

    for (int i = 3; i < argc; ++i) {
        std::string spec = argv[i];
        // split by ':'
        auto parts = split(spec, ':'); // use your Utility::split(string, char)
        if (parts.size() < 2) {
            std::cout << "Invalid column spec: " << spec << "\n";
            return;
        }
        ColumnDef c;
        c.name = parts[0];
        try {
            c.type = getDataType(parts[1]); // your existing mapper
        } catch (...) {
            std::cout << "Invalid type in: " << spec << "\n";
            return;
        }
        for (size_t k = 2; k < parts.size(); ++k) {
            if (parts[k] == "pk") c.isPrimaryKey = true;
            else if (parts[k] == "notnull") c.notNull = true;
            else if (parts[k].rfind("fk=", 0) == 0) {
                auto fk = parts[k].substr(3);
                auto pair = split(fk, '.');
                if (pair.size() == 2) {
                    c.hasForeignKey = true;
                    c.fkTable  = pair[0];
                    c.fkColumn = pair[1];
                } else {
                    std::cout << "Invalid fk format in: " << spec << " (use fk=Table.Column)\n";
                    return;
                }
            } else {
                std::cout << "Unknown modifier in: " << spec << " (use pk/notnull/fk=...)\n";
                return;
            }
        }
        tdef.columns.push_back(c);
    }

    // Load & update catalog
    Catalog cat = Catalog::load();
    if (cat.tableExists(tableName)) {
        std::cout << "Table already exists: " << tableName << "\n";
        return;
    }

    // (Optional) Validate FK targets exist right now (stronger UX)
    for (const auto& col : tdef.columns) {
        if (col.hasForeignKey) {
            auto tgt = cat.getTable(col.fkTable);
            if (!tgt.has_value()) {
                std::cout << "FK references missing table: " << col.fkTable << "\n";
                return;
            }
            bool foundCol = false;
            for (const auto& tc : tgt->columns) {
                if (tc.name == col.fkColumn) { foundCol = true; break; }
            }
            if (!foundCol) {
                std::cout << "FK references missing column: " << col.fkTable << "." << col.fkColumn << "\n";
                return;
            }
        }
    }

    if (!cat.addTable(tdef) || !cat.save()) {
        std::cout << "Failed to register table in catalog.\n";
        return;
    }

    // Create empty data file
    makeDir("data"); // use same helper as before
    std::ofstream dataFile(("data/" + tableName + ".dat").c_str(), std::ios::app);
    dataFile.close();

    // Create per-table schema snapshot if you still use it elsewhere (optional)
    // Or you can migrate all schema reads to Catalog instead of Schema::<...>

    std::cout << "Relational table '" << tableName << "' created and registered.\n";
}



else if (command == "dikhao") {
    if (argc < 3) {
        std::cout << "Usage: cdb dikhao <table> [where <col> (=|like) <value>]\n";
        return;
    }

    std::string tableName = argv[2];
    Schema schema;
    try {
        schema = Schema::loadFromFile(tableName);
    } catch (const std::exception& e) {
        std::cout << "Failed to load schema for table: " << tableName << "\n";
        return;
    }

    std::ifstream dataFile("data/" + tableName + ".dat");
    if (!dataFile.is_open()) {
        std::cout << "Failed to open data file for table: " << tableName << "\n";
        return;
    }

    bool useFilter = false;
    std::string whereCol, whereOp, whereVal;

    if (argc > 3 && std::string(argv[3]) == "where") {
        if (argc == 7) {
            useFilter = true;
            whereCol = argv[4];
            whereOp = argv[5];
            whereVal = argv[6];
        } else {
            std::cout << "Invalid WHERE clause syntax.\n";
            return;
        }
    }

    int whereColIdx = -1;
    if (useFilter) {
        const auto& columns = schema.getColumns();
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == whereCol) {
                whereColIdx = static_cast<int>(i);
                break;
            }
        }
        if (whereColIdx == -1) {
            std::cout << "Column not found in schema: " << whereCol << "\n";
            return;
        }
    }

    const auto& columns = schema.getColumns();
    std::vector<std::vector<std::string>> allRows;

    std::string line;
    while (std::getline(dataFile, line)) {
        auto values = split(line, ',');  
        if (values.size() != columns.size()) {
            std::cout << "Skipping malformed row: " << line << "\n";
            continue;
        }
        allRows.push_back(values);
    }
    dataFile.close();

    std::vector<size_t> colWidths(columns.size());
    for (size_t i = 0; i < columns.size(); ++i) {
        colWidths[i] = columns[i].name.size();
    }
    for (const auto& row : allRows) {
        for (size_t i = 0; i < row.size(); ++i) {
            if (row[i].size() > colWidths[i]) {
                colWidths[i] = row[i].size();
            }
        }
    }

    auto printSeparator = [&]() {
        for (auto w : colWidths) {
            std::cout << "+" << std::string(w + 2, '-');
        }
        std::cout << "+\n";
    };

    printSeparator();
    for (size_t i = 0; i < columns.size(); ++i) {
        std::cout << "| " << std::left << std::setw(colWidths[i]) << columns[i].name << " ";
    }
    std::cout << "|\n";
    printSeparator();

    for (const auto& row : allRows) {
        bool match = true;
        if (useFilter) {
            std::string cell = row[whereColIdx];
            if (whereOp == "=") {
                match = (cell == whereVal);
            } else if (whereOp == "like") {
                match = (cell.find(whereVal) != std::string::npos);
            } else {
                std::cout << "Unsupported operator: " << whereOp << "\n";
                return;
            }
        }

        if (match) {
            for (size_t i = 0; i < row.size(); ++i) {
                std::cout << "| " << std::left << std::setw(colWidths[i]) << row[i] << " ";
            }
            std::cout << "|\n";
        }
    }
    printSeparator();
}
else if (command == "update_karo") {
    if (argc < 5 || std::string(argv[3]) != "change") {
        std::cout << "Usage: cdb update_karo <table> change <col>=<val> [where <col> (=|like) <val>]\n";
        return;
    }

    std::string tableName = argv[2];
    std::string setArg = argv[4];

    auto setParts = split(setArg, '=');
    if (setParts.size() != 2) {
        std::cout << "Invalid change format. Use <col>=<val>\n";
        return;
    }

    std::string setCol = setParts[0];
    std::string setVal = setParts[1];


    bool useFilter = false;
    std::string whereCol, whereOp, whereVal;
    if (argc >= 9 && std::string(argv[5]) == "where") {
        useFilter = true;
        whereCol = argv[6];
        whereOp = argv[7];
        whereVal = argv[8];
    }

    Schema schema;
    try {
        schema = Schema::loadFromFile(tableName);
    } catch (...) {
        std::cout << "Failed to load schema.\n";
        return;
    }

    const auto& columns = schema.getColumns();
    int setColIdx = -1, whereColIdx = -1;

    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == setCol) setColIdx = static_cast<int>(i);
        if (useFilter && columns[i].name == whereCol) whereColIdx = static_cast<int>(i);
    }

    if (setColIdx == -1) {
        std::cout << "Column to change not found in schema: " << setCol << "\n";
        return;
    }
    if (useFilter && whereColIdx == -1) {
        std::cout << "WHERE column not found in schema: " << whereCol << "\n";
        return;
    }

    std::ifstream inFile("data/" + tableName + ".dat");
    if (!inFile.is_open()) {
        std::cout << "Failed to open data file.\n";
        return;
    }

    std::vector<std::vector<std::string>> updatedRows;
    std::string line;
    int updateCount = 0;

    while (std::getline(inFile, line)) {
        auto values = split(line, ',');
        if (values.size() != columns.size()) continue;

        bool match = true;
        if (useFilter) {
            std::string cell = values[whereColIdx];
            if (whereOp == "=") {
                match = (cell == whereVal);
            } else if (whereOp == "like") {
                match = cell.find(whereVal) != std::string::npos;
            } else {
                std::cout << "Unsupported WHERE operator: " << whereOp << "\n";
                return;
            }
        }

        if (match) {
            values[setColIdx] = setVal;
            updateCount++;
        }

        updatedRows.push_back(values);
    }
    inFile.close();

    std::ofstream outFile("data/" + tableName + ".dat");
    for (const auto& row : updatedRows) {
        for (size_t i = 0; i < row.size(); ++i) {
            outFile << row[i];
            if (i != row.size() - 1) outFile << ",";
        }
        outFile << "\n";
    }
    outFile.close();

    std::cout << "Updated " << updateCount << " row(s).\n";
}
else if (command == "delete_karo") {
    if (argc < 3) {
        std::cout << "Usage: cdb delete_karo <table> [where <col> (=|like) <val>]\n";
        return;
    }

    std::string tableName = argv[2];

    bool useFilter = false;
    std::string whereCol, whereOp, whereVal;

    if (argc >= 6 && std::string(argv[3]) == "where") {
        if (argc == 6 + 1) { 
            useFilter = true;
            whereCol = argv[4];
            whereOp = argv[5];
            whereVal = argv[6];
        } else {
            std::cout << "Invalid WHERE clause syntax.\n";
            return;
        }
    }

    Schema schema;
    try {
        schema = Schema::loadFromFile(tableName);
    } catch (...) {
        std::cout << "Failed to load schema for table: " << tableName << "\n";
        return;
    }

    const auto& columns = schema.getColumns();
    int whereColIdx = -1;

    if (useFilter) {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (columns[i].name == whereCol) {
                whereColIdx = static_cast<int>(i);
                break;
            }
        }
        if (whereColIdx == -1) {
            std::cout << "WHERE column not found in schema: " << whereCol << "\n";
            return;
        }
    }

    std::ifstream inFile("data/" + tableName + ".dat");
    if (!inFile.is_open()) {
        std::cout << "Failed to open data file.\n";
        return;
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;
    int deleteCount = 0;

    while (std::getline(inFile, line)) {
        auto values = split(line, ',');
        if (values.size() != columns.size()) continue;

        bool match = true;
        if (useFilter) {
            std::string cell = values[whereColIdx];
            if (whereOp == "=") {
                match = (cell == whereVal);
            } else if (whereOp == "like") {
                match = cell.find(whereVal) != std::string::npos;
            } else {
                std::cout << "Unsupported WHERE operator: " << whereOp << "\n";
                return;
            }
        }

        if (match) {
            deleteCount++;
        } else {
            rows.push_back(values);
        }
    }
    inFile.close();

    if (!useFilter) {
        std::string confirm;
        std::cout << "Are you sure you want to delete ALL records from table '" << tableName << "'? (yes/no): ";
        std::getline(std::cin, confirm);
        if (confirm != "yes") {
            std::cout << "Deletion cancelled.\n";
            return;
        }
    }

    std::ofstream outFile("data/" + tableName + ".dat", std::ios::trunc);
    for (const auto& row : rows) {
        for (size_t i = 0; i < row.size(); ++i) {
            outFile << row[i];
            if (i != row.size() - 1) outFile << ",";
        }
        outFile << "\n";
    }
    outFile.close();

    std::cout << "Deleted " << deleteCount << " row(s).\n";
}
else if (command == "drop_kro_table") {
    if (argc < 3) {
        std::cout << "Usage: cdb drop_kro_table <table>\n";
        return;
    }

    std::string tableName = argv[2];
    std::string metaPath = "metadata/" + tableName + ".meta";
    std::string dataPath = "data/" + tableName + ".dat";

    std::string confirm;
    std::cout << "Are you sure you want to permanently delete the table '" << tableName << "'? (yes/no): ";
    std::getline(std::cin, confirm);

    if (confirm != "yes") {
        std::cout << "Table drop cancelled.\n";
        return;
    }

    // Delete schema file
    if (std::remove(metaPath.c_str()) != 0) {
        std::perror(("Failed to delete metadata file: " + metaPath).c_str());
    } else {
        std::cout << "Deleted metadata file.\n";
    }

    // Delete data file
    if (std::remove(dataPath.c_str()) != 0) {
        std::perror(("Failed to delete data file: " + dataPath).c_str());
    } else {
        std::cout << "Deleted data file.\n";
    }

    std::cout << "Table '" << tableName << "' dropped successfully.\n";
}

else if (command == "describe_kro") {
    if (argc < 3) {
        std::cout << "Usage: cdb describe_kro <table>\n";
        return;
    }

    std::string tableName = argv[2];

    Schema schema;
    try {
        schema = Schema::loadFromFile(tableName);
    } catch (const std::exception& e) {
        std::cout << "Failed to load schema for table: " << tableName << "\n";
        return;
    }

    const auto& columns = schema.getColumns();
    std::cout << "+----------------+------------+\n";
    std::cout << "| Column Name    | Type       |\n";
    std::cout << "+----------------+------------+\n";
    for (const auto& col : columns) {
        std::cout << "| " << std::setw(14) << std::left << col.name
                  << " | " << std::setw(10) << std::left;
        switch (col.type) {
            case DataType::INT: std::cout << "INT"; break;
            case DataType::STRING: std::cout << "STRING"; break;
            case DataType::FLOAT: std::cout << "FLOAT"; break;
            default: std::cout << "UNKNOWN"; break;
        }
        std::cout << " |\n";
    }
    std::cout << "+----------------+------------+\n";
}




    else {
        std::cout << "Unknown command: " << command << "\n";
    }
}

void CommandHandler::execute(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: cdb <command> [args...]\n";
        return;
    }

    std::string command = argv[1];
    handleCommand(argc, argv, command);
}
