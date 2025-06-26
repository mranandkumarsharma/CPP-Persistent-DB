#include "CommandHandler.hpp"
#include "Schema.hpp"
#include "Utility.hpp"
#include <iostream>
#include <fstream>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif

void makeDir(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

void handleCommand(int argc, char* argv[], const std::string& command) {
    if (command == "create_table") {
        if (argc < 4) {
            std::cout << "Usage: cdb create_table <table> <col1:type1> <col2:type2> ...\n";
            return;
        }

        std::string tableName = argv[2];
        std::vector<Column> columns;

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            auto parts = split(arg, ':');
            if (parts.size() != 2) {
                std::cout << "Invalid column format: " << arg << "\n";
                return;
            }

            Column col;
            col.name = parts[0];
            try {
                col.type = getDataType(parts[1]);
            } catch (...) {
                std::cout << "Invalid type: " << parts[1] << "\n";
                return;
            }

            columns.push_back(col);
        }

        Schema schema(columns);
        makeDir("metadata");
        makeDir("data");

        if (!schema.saveToFile(tableName)) {
            std::cout << "Failed to save schema.\n";
            return;
        }

        std::ofstream dataFile("data/" + tableName + ".dat");
        dataFile.close();

        std::cout << "Table '" << tableName << "' created successfully.\n";
    }

    else if (command == "insert") {
        if (argc < 4) {
            std::cout << "Usage: cdb insert <table> <val1> <val2> ...\n";
            return;
        }

        std::string tableName = argv[2];
        Schema schema;
    try {
        schema = Schema::loadFromFile(tableName);
    } catch (const std::exception& e) {
        std::cout << "Schema for table '" << tableName << "' not found.\n";
        return;
    }


        const std::vector<Column>& columns = schema.getColumns();
        if ((argc - 3) != columns.size()) {
            std::cout << "Expected " << columns.size() << " values, got " << (argc - 3) << ".\n";
            return;
        }

        std::ofstream out("data/" + tableName + ".dat", std::ios::app);
        if (!out.is_open()) {
            std::cout << "Failed to open data file for table '" << tableName << "'.\n";
            return;
        }

        for (size_t i = 0; i < columns.size(); ++i) {
            std::string val = argv[3 + i];
            DataType expected = columns[i].type;

            if (expected == DataType::INT) {
                try { std::stoi(val); } catch (...) {
                    std::cout << "Invalid int value for column '" << columns[i].name << "': " << val << "\n";
                    return;
                }
            } else if (expected == DataType::FLOAT) {
                try { std::stof(val); } catch (...) {
                    std::cout << "Invalid float value for column '" << columns[i].name << "': " << val << "\n";
                    return;
                }
            }

            out << val;
            if (i < columns.size() - 1)
                out << ",";
        }

        out << "\n";
        out.close();
        std::cout << "Inserted 1 row into '" << tableName << "'.\n";
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
