#pragma once
#include <string>

enum class DataType { INT, STRING, FLOAT };

DataType getDataType(const std::string& typeStr);
std::string toString(DataType type);
