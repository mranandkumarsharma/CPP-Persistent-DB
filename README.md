# CDB — Persistent Database in C++

A simple persistent database implemented in C++ with basic CRUD operations, schema management, and simple query support including optional WHERE clauses with `=` and `like` operators.

---

## Build Instructions

You need **CMake** and **MinGW** (or compatible GCC) installed.

```bash
cmake .. -G "MinGW Makefiles"
mingw32-make
```

General Usage
Run the executable from the command line with commands and arguments:
```bash
cdb <command> <arguments...>
```
1. Create Table(table_banao)
Create a new table with specified columns and data types.
```bash
cdb table_banao <table_name> <col1:type1> <col2:type2> ...
```
Example:
```bash
cdb table_banao users id:int name:string age:int
```
2. Insert Data(insert_karo)
Insert a new row into a table.
```bash
cdb insert_karo <table_name> <value1> <value2> ...
```
Values should be in the same order as the columns defined in the schema.
Example:
```bash
cdb insert_karo users 1 Alice 23
```
3. Retrieve Data (dikhao)
Display rows from a table optionally filtered by a WHERE clause.
```bash
cdb dikhao <table_name> [where <column> (=|like) <value>]
```
