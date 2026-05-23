#include "printer.hpp"

#include <functional>

namespace semantic {

namespace {

std::string literal_kind_name(LiteralKind kind) {
    switch (kind) {
        case LiteralKind::None: return "None";
        case LiteralKind::Integer: return "Integer";
        case LiteralKind::Real: return "Real";
        case LiteralKind::Char: return "Char";
        case LiteralKind::String: return "String";
        case LiteralKind::Boolean: return "Boolean";
        case LiteralKind::IdentifierConstant: return "IdentifierConstant";
    }
    return "Unknown";
}

void print_node_label(std::ostream& os, const AstNode& node, const TypeRegistry& types) {
    os << ast_kind_name(node.kind);
    if (!node.name.empty()) os << "(" << node.name << ")";
    if (!node.value.empty()) os << " value=" << node.value;
    if (!node.op.empty()) os << " op=" << node.op;
    if (node.literal_kind != LiteralKind::None) {
        os << " literal=" << literal_kind_name(node.literal_kind);
    }
    if (node.annotation.type_id != 0) {
        os << " : type=" << types.type_name(node.annotation.type_id);
    }
    if (node.annotation.tab_index >= 0) {
        os << ", tab_index=" << node.annotation.tab_index;
    }
    if (node.annotation.lexical_level != 0) {
        os << ", lev=" << node.annotation.lexical_level;
    }
    if (node.annotation.block_index != 0) {
        os << ", block=" << node.annotation.block_index;
    }
}

const char* severity_name(DiagnosticSeverity severity) {
    return severity == DiagnosticSeverity::Warning ? "Warning" : "Error";
}

} // namespace

void print_decorated_ast(std::ostream& os, const AstNodePtr& root, const TypeRegistry& types) {
    if (!root) {
        os << "<empty>\n";
        return;
    }

    print_node_label(os, *root, types);
    os << '\n';

    std::function<void(const AstNodePtr&, const std::string&, bool)> print_child =
        [&](const AstNodePtr& node, const std::string& prefix, bool is_last) {
            if (!node) return;
            os << prefix << (is_last ? "\\-- " : "|-- ");
            print_node_label(os, *node, types);
            os << '\n';

            const std::string next_prefix = prefix + (is_last ? "    " : "|   ");
            for (size_t i = 0; i < node->children.size(); ++i) {
                print_child(node->children[i], next_prefix, i + 1 == node->children.size());
            }
        };

    for (size_t i = 0; i < root->children.size(); ++i) {
        print_child(root->children[i], "", i + 1 == root->children.size());
    }
}

void print_symbol_tables(std::ostream& os, const SymbolTable& symbols, const TypeRegistry& types) {
    os << "\n=== Symbol Table: tab ===\n";
    os << "idx\tidentifier\tobj\ttype\tref\tnrm\tlev\tadr\tlink\tinit\tvalue\n";
    const auto& tab = symbols.tab_entries();
    for (size_t i = 0; i < tab.size(); ++i) {
        const auto& entry = tab[i];
        os << i << '\t' << entry.identifier << '\t' << symbol_object_name(entry.obj) << '\t'
           << types.type_name(entry.type) << '\t' << entry.ref << '\t' << entry.nrm << '\t'
           << entry.lev << '\t' << entry.adr << '\t' << entry.link << '\t'
           << (entry.initialized ? "yes" : "no") << '\t' << entry.value << '\n';
    }

    os << "\n=== Symbol Table: btab ===\n";
    os << "idx\tlast\tlpar\tpsze\tvsze\n";
    const auto& btab = symbols.btab_entries();
    for (size_t i = 0; i < btab.size(); ++i) {
        os << i << '\t' << btab[i].last << '\t' << btab[i].lpar << '\t'
           << btab[i].psze << '\t' << btab[i].vsze << '\n';
    }

    os << "\n=== Symbol Table: atab ===\n";
    os << "idx\txtyp\tetyp\teref\tlow\thigh\telsz\tsize\n";
    const auto& atab = symbols.atab_entries();
    for (size_t i = 0; i < atab.size(); ++i) {
        os << i << '\t' << types.type_name(atab[i].xtyp) << '\t'
           << types.type_name(atab[i].etyp) << '\t' << atab[i].eref << '\t'
           << atab[i].low << '\t' << atab[i].high << '\t'
           << atab[i].elsz << '\t' << atab[i].size << '\n';
    }
}

void print_diagnostics(std::ostream& os, const std::vector<Diagnostic>& diagnostics) {
    os << "\n=== Semantic Errors ===\n";
    if (diagnostics.empty()) {
        os << "No semantic errors.\n";
        return;
    }

    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.location.line > 0 || diagnostic.location.column > 0) {
            os << diagnostic.location.line << ':' << diagnostic.location.column << ": ";
        }
        os << severity_name(diagnostic.severity) << ": " << diagnostic.message << '\n';
    }
}

void print_semantic_report(std::ostream& os, const AstNodePtr& root,
                           const SymbolTable& symbols,
                           const TypeRegistry& types,
                           const std::vector<Diagnostic>& diagnostics) {
    os << "\n=== Decorated AST ===\n";
    print_decorated_ast(os, root, types);
    print_symbol_tables(os, symbols, types);
    print_diagnostics(os, diagnostics);
}

} // namespace semantic
