#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "types.hpp"

namespace semantic {

enum class SymbolObject {
    Reserved,
    Constant,
    Variable,
    Type,
    Procedure,
    Function,
    Program,
    Parameter,
    Field
};

struct TabEntry {
    std::string identifier;
    SymbolObject obj = SymbolObject::Variable;
    int type = 0;
    int ref = 0;
    int nrm = 1;
    int lev = 0;
    int adr = 0;
    int link = 0;
    bool initialized = false;
    std::string value;
};

struct BTabEntry {
    int last = 0;
    int lpar = 0;
    int psze = 0;
    int vsze = 0;
};

struct ATabEntry {
    int xtyp = 0;
    int etyp = 0;
    int eref = 0;
    int low = 0;
    int high = 0;
    int elsz = 1;
    int size = 0;
};

class SymbolTable {
public:
    SymbolTable() = default;

    void initialize_predefined(const TypeRegistry& types);
    void push_scope(int block_index);
    void pop_scope();
    int add_block();
    int current_level() const;
    int current_block_index() const;

    int raw_add_symbol(const std::string& id, SymbolObject obj, int type,
                       int ref, int nrm, int lev, int adr,
                       bool initialized = true, const std::string& value = "");
    int add_symbol(const std::string& id, SymbolObject obj, int type,
                   int ref = 0, int nrm = 1, bool initialized = false,
                   const std::string& value = "", int size = 1);

    int lookup(const std::string& id) const;
    int lookup_current_scope(const std::string& id) const;
    int lookup_global_const_or_var(const std::string& id) const;

    int add_array_entry(int index_type, int element_type, int low, int high,
                        int element_size, int element_ref);

    TabEntry& tab_mutable(int index);
    const TabEntry& tab_entry(int index) const;
    BTabEntry& btab_mutable(int index);
    const ATabEntry& atab_entry(int index) const;

    const std::vector<TabEntry>& tab_entries() const;
    const std::vector<BTabEntry>& btab_entries() const;
    const std::vector<ATabEntry>& atab_entries() const;

    static std::string normalize(std::string identifier);

private:
    std::vector<TabEntry> tab;
    std::vector<BTabEntry> btab;
    std::vector<ATabEntry> atab;
    std::vector<std::unordered_map<std::string, int>> scopes;
    std::unordered_map<std::string, int> predefined_symbols;
    std::vector<int> scope_last;
    std::vector<int> display;
    std::vector<int> next_address;
};

std::string symbol_object_name(SymbolObject obj);

} // namespace semantic
