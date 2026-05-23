#include "symbol_table.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace semantic {

std::string SymbolTable::normalize(std::string identifier) {
    std::transform(identifier.begin(), identifier.end(), identifier.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return identifier;
}

void SymbolTable::initialize_predefined(const TypeRegistry& types) {
    tab.clear();
    btab.clear();
    atab.clear();
    scopes.clear();
    scope_last.clear();
    display.clear();
    next_address.clear();

    btab.push_back({});
    push_scope(0);

    const std::vector<std::string> reserved = {
        "and", "array", "begin", "case", "const", "div", "downto", "do",
        "else", "end", "for", "function", "if", "mod", "not", "of", "or",
        "procedure", "program", "record", "repeat", "integer", "real",
        "boolean", "char", "string", "then", "to", "type", "until", "var",
        "while"
    };

    for (const auto& word : reserved) {
        raw_add_symbol(word, SymbolObject::Reserved, 0, 0, 1, 0, 0);
    }

    raw_add_symbol("Integer", SymbolObject::Type, types.integer_type(), 0, 1, 0, 0);
    raw_add_symbol("Real", SymbolObject::Type, types.real_type(), 0, 1, 0, 0);
    raw_add_symbol("Char", SymbolObject::Type, types.char_type(), 0, 1, 0, 0);
    raw_add_symbol("Boolean", SymbolObject::Type, types.boolean_type(), 0, 1, 0, 0);
    raw_add_symbol("String", SymbolObject::Type, types.string_type(), 0, 1, 0, 0);
    raw_add_symbol("True", SymbolObject::Constant, types.boolean_type(), 0, 1, 0, 1, true, "true");
    raw_add_symbol("False", SymbolObject::Constant, types.boolean_type(), 0, 1, 0, 0, true, "false");
    raw_add_symbol("readln", SymbolObject::Procedure, 0, 0, 1, 0, 0);
    raw_add_symbol("writeln", SymbolObject::Procedure, 0, 0, 1, 0, 0);
}

void SymbolTable::push_scope(int block_index) {
    scopes.push_back({});
    scope_last.push_back(0);
    display.push_back(block_index);
    next_address.push_back(0);
}

void SymbolTable::pop_scope() {
    if (!scopes.empty()) scopes.pop_back();
    if (!scope_last.empty()) scope_last.pop_back();
    if (!display.empty()) display.pop_back();
    if (!next_address.empty()) next_address.pop_back();
}

int SymbolTable::add_block() {
    btab.push_back({});
    return static_cast<int>(btab.size()) - 1;
}

int SymbolTable::current_level() const {
    return scopes.empty() ? 0 : static_cast<int>(scopes.size()) - 1;
}

int SymbolTable::current_block_index() const {
    return display.empty() ? 0 : display.back();
}

int SymbolTable::raw_add_symbol(const std::string& id, SymbolObject obj, int type,
                                int ref, int nrm, int lev, int adr,
                                bool initialized, const std::string& value) {
    const int index = static_cast<int>(tab.size());
    const int link = scope_last.empty() ? 0 : scope_last.back();
    tab.push_back({id, obj, type, ref, nrm, lev, adr, link, initialized, value});

    if (!scopes.empty()) {
        scopes.back()[normalize(id)] = index;
        scope_last.back() = index;
        const int block_index = current_block_index();
        if (block_index >= 0 && static_cast<size_t>(block_index) < btab.size()) {
            btab[static_cast<size_t>(block_index)].last = index;
        }
    }

    return index;
}

int SymbolTable::add_symbol(const std::string& id, SymbolObject obj, int type,
                            int ref, int nrm, bool initialized,
                            const std::string& value, int size) {
    const int adr = next_address.empty() ? 0 : next_address.back();
    if (obj == SymbolObject::Variable) {
        if (!next_address.empty()) next_address.back() += size;
        const int block_index = current_block_index();
        if (block_index >= 0 && static_cast<size_t>(block_index) < btab.size()) {
            btab[static_cast<size_t>(block_index)].vsze += size;
        }
    }

    return raw_add_symbol(id, obj, type, ref, nrm, current_level(), adr, initialized, value);
}

int SymbolTable::lookup(const std::string& id) const {
    const std::string key = normalize(id);
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(key);
        if (found != it->end()) return found->second;
    }
    return -1;
}

int SymbolTable::lookup_current_scope(const std::string& id) const {
    if (scopes.empty()) return -1;
    const auto found = scopes.back().find(normalize(id));
    return found == scopes.back().end() ? -1 : found->second;
}

int SymbolTable::lookup_global_const_or_var(const std::string& id) const {
    const std::string key = normalize(id);
    for (size_t i = 0; i < tab.size(); ++i) {
        const auto& entry = tab[i];
        if (entry.lev == 0 &&
            (entry.obj == SymbolObject::Constant || entry.obj == SymbolObject::Variable) &&
            normalize(entry.identifier) == key) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int SymbolTable::add_array_entry(int index_type, int element_type, int low, int high,
                                 int element_size, int element_ref) {
    const int count = high >= low ? high - low + 1 : 0;
    const int index = static_cast<int>(atab.size());
    atab.push_back({index_type, element_type, element_ref, low, high,
                    element_size, count * element_size});
    return index;
}

TabEntry& SymbolTable::tab_mutable(int index) {
    if (index < 0 || static_cast<size_t>(index) >= tab.size()) {
        throw std::out_of_range("invalid tab index");
    }
    return tab[static_cast<size_t>(index)];
}

const TabEntry& SymbolTable::tab_entry(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= tab.size()) {
        throw std::out_of_range("invalid tab index");
    }
    return tab[static_cast<size_t>(index)];
}

BTabEntry& SymbolTable::btab_mutable(int index) {
    if (index < 0 || static_cast<size_t>(index) >= btab.size()) {
        throw std::out_of_range("invalid btab index");
    }
    return btab[static_cast<size_t>(index)];
}

const ATabEntry& SymbolTable::atab_entry(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= atab.size()) {
        throw std::out_of_range("invalid atab index");
    }
    return atab[static_cast<size_t>(index)];
}

const std::vector<TabEntry>& SymbolTable::tab_entries() const {
    return tab;
}

const std::vector<BTabEntry>& SymbolTable::btab_entries() const {
    return btab;
}

const std::vector<ATabEntry>& SymbolTable::atab_entries() const {
    return atab;
}

std::string symbol_object_name(SymbolObject obj) {
    switch (obj) {
        case SymbolObject::Reserved: return "reserved";
        case SymbolObject::Constant: return "constant";
        case SymbolObject::Variable: return "variable";
        case SymbolObject::Type: return "type";
        case SymbolObject::Procedure: return "procedure";
        case SymbolObject::Function: return "function";
        case SymbolObject::Program: return "program";
        case SymbolObject::Parameter: return "parameter";
        case SymbolObject::Field: return "field";
    }
    return "unknown";
}

} // namespace semantic
