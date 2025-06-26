#include "DataType.hpp"
#include <stdexcept>

DataType getDataType(const std::string& typeStr) {
    if (typeStr == "int") return DataType::INT;
    if (typeStr == "string") return DataType::STRING;
    if (typeStr == "float") return DataType::FLOAT;
    throw std::invalid_argument("Unknown data type: " + typeStr);
}

std::string toString(DataType type) {
    switch (type) {
        case DataType::INT: return "int";
        case DataType::STRING: return "string";
        case DataType::FLOAT: return "float";
        default: return "unknown";
    }
}
