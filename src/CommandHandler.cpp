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

// Internal helper function (not part of the class)
void handleCommand(int argc, char* argv[], const std::string& command) {
    // Your command handling code here as before...
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
            } catch (const std::exception& e) {
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
    } else {
        std::cout << "Unknown command: " << command << "\n";
    }
}

// Now the actual class method definition
void CommandHandler::execute(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: cdb <command> [args...]\n";
        return;
    }

    std::string command = argv[1];
    handleCommand(argc, argv, command);
}
