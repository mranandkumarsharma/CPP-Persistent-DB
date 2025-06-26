#include "Utility.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> tokens;
    while (getline(ss, item, delimiter)) {
        tokens.push_back(trim(item));
    }
    return tokens;
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    size_t end = s.find_last_not_of(" \t\n\r");
    return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
}
