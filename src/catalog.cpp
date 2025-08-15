#include "Catalog.hpp"
#include "Utility.hpp"   // for trim/split if you have them; else add small helpers here
#include <fstream>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
  #include <direct.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
#endif

// ---- small helpers if not in Utility.hpp ----
static std::string ltrim(std::string s) {
    size_t i=0; while (i<s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\r')) ++i; return s.substr(i);
}
static std::string rtrim(std::string s) {
    if (s.empty()) return s;
    size_t i=s.size()-1; while (i!=std::string::npos && (s[i]==' '||s[i]=='\t'||s[i]=='\r')) { if (i==0){return "";} --i; }
    return s.substr(0, i+1);
}
static std::string trimString(std::string s){ return rtrim(ltrim(s)); }

static void makeDir(const std::string& path) {
#ifdef _WIN32
    _mkdir(path.c_str());
#else
    mkdir(path.c_str(), 0755);
#endif
}

static std::vector<std::string> splitBy(const std::string& s, char delim) {
    std::vector<std::string> out; std::stringstream ss(s); std::string item;
    while (std::getline(ss, item, delim)) out.push_back(item);
    return out;
}
// ---------------------------------------------

std::string Catalog::catalogPath() {
    return "metadata/catalog.meta";
}
void Catalog::ensureMetadataDir() {
    makeDir("metadata");
}

static std::string dataTypeToString(DataType t) {
    switch (t) {
        case DataType::INT: return "INT";
        case DataType::STRING: return "STRING";
        case DataType::FLOAT: return "FLOAT";
        default: return "UNKNOWN";
    }
}

// Serialize a simple human-readable format.
// Example:
//
// [table users]
// col id INT pk notnull
// col name STRING
// col age INT
// end
//
// [table orders]
// col order_id INT pk
// col user_id INT fk=users.id
// end
//
std::string Catalog::serialize(const Catalog& c) {
    std::ostringstream out;
    for (const auto& t : c.tables) {
        out << "[table " << t.name << "]\n";
        for (const auto& col : t.columns) {
            out << "col " << col.name << " " << dataTypeToString(col.type);
            if (col.isPrimaryKey) out << " pk";
            if (col.notNull) out << " notnull";
            if (col.hasForeignKey) out << " fk=" << col.fkTable << "." << col.fkColumn;
            out << "\n";
        }
        out << "end\n";
    }
    return out.str();
}

static DataType parseType(const std::string& s) {
    // Reuse your getDataType if you prefer; here’s a simple mapper:
    std::string u = s; for (auto& ch : u) ch = toupper(ch);
    if (u == "INT") return DataType::INT;
    if (u == "STRING") return DataType::STRING;
    if (u == "FLOAT") return DataType::FLOAT;
    return DataType::STRING; // fallback
}

Catalog Catalog::parse(const std::string& text) {
    Catalog cat;
    std::istringstream in(text);
    std::string line;
    TableDef current;
    bool inTable = false;

    while (std::getline(in, line)) {
        line = trimString(line);
        if (line.empty()) continue;

        if (line.rfind("[table ", 0) == 0 && line.back() == ']') {
            if (inTable) { /* malformed; ignore */ }
            inTable = true;
            current = TableDef{};
            current.name = trimString(line.substr(7, line.size() - 8)); // inside [table ...]
            continue;
        }
        if (line == "end") {
            if (inTable) {
                cat.tables.push_back(current);
                inTable = false;
            }
            continue;
        }
        if (inTable) {
            // col <name> <TYPE> [pk] [notnull] [fk=t.c]
            if (line.rfind("col ", 0) == 0) {
                auto rest = trimString(line.substr(4));
                auto tokens = splitBy(rest, ' ');
                if (tokens.size() >= 2) {
                    ColumnDef cd;
                    cd.name = tokens[0];
                    cd.type = parseType(tokens[1]);

                    for (size_t i = 2; i < tokens.size(); ++i) {
                        if (tokens[i] == "pk") cd.isPrimaryKey = true;
                        else if (tokens[i] == "notnull") cd.notNull = true;
                        else if (tokens[i].rfind("fk=", 0) == 0) {
                            auto fk = tokens[i].substr(3);
                            auto parts = splitBy(fk, '.');
                            if (parts.size() == 2) {
                                cd.hasForeignKey = true;
                                cd.fkTable = parts[0];
                                cd.fkColumn = parts[1];
                            }
                        }
                    }
                    current.columns.push_back(cd);
                }
            }
        }
    }
    return cat;
}

Catalog Catalog::load() {
    ensureMetadataDir();
    std::ifstream in(catalogPath());
    if (!in.is_open()) {
        // create empty file
        std::ofstream out(catalogPath());
        out.close();
        return Catalog{};
    }
    std::ostringstream buf;
    buf << in.rdbuf();
    in.close();
    return parse(buf.str());
}

bool Catalog::save() const {
    ensureMetadataDir();
    std::ofstream out(catalogPath(), std::ios::trunc);
    if (!out.is_open()) return false;
    out << serialize(*this);
    out.close();
    return true;
}

bool Catalog::addTable(const TableDef& tdef) {
    if (tableExists(tdef.name)) return false;
    tables.push_back(tdef);
    return true;
}

std::optional<TableDef> Catalog::getTable(const std::string& name) const {
    for (const auto& t : tables) if (t.name == name) return t;
    return std::nullopt;
}

bool Catalog::tableExists(const std::string& name) const {
    for (const auto& t : tables) if (t.name == name) return true;
    return false;
}

std::vector<std::string> Catalog::listTables() const {
    std::vector<std::string> out;
    out.reserve(tables.size());
    for (const auto& t : tables) out.push_back(t.name);
    return out;
}
