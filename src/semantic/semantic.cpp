#include "semantic.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

DecoratedAstNode::DecoratedAstNode(std::string label) : label(std::move(label)) {}

void DecoratedAstNode::add_child(const std::shared_ptr<DecoratedAstNode>& child) {
    if (child) {
        children.push_back(child);
    }
}

void DecoratedAstNode::print(std::ostream& os, const std::vector<SemanticType>& types) const {
    auto type_label = [&](int type_index) {
        if (type_index > 0 && static_cast<size_t>(type_index) < types.size()) {
            return types[static_cast<size_t>(type_index)].name;
        }
        return std::string("-");
    };

    std::function<void(const DecoratedAstNode&, const std::string&, bool)> print_node =
        [&](const DecoratedAstNode& node, const std::string& prefix, bool is_last) {
            os << prefix << (is_last ? "\\-- " : "|-- ") << node.label;
            if (node.type != 0) os << " : type=" << type_label(node.type);
            if (node.tab_index != 0) os << ", tab_index=" << node.tab_index;
            if (node.level != 0) os << ", lev=" << node.level;
            if (node.block_index != 0) os << ", block=" << node.block_index;
            os << '\n';

            const std::string next_prefix = prefix + (is_last ? "    " : "|   ");
            for (size_t i = 0; i < node.children.size(); ++i) {
                print_node(*node.children[i], next_prefix, i + 1 == node.children.size());
            }
        };

    os << label;
    if (type != 0) os << " : type=" << type_label(type);
    if (tab_index != 0) os << ", tab_index=" << tab_index;
    if (block_index != 0) os << ", block=" << block_index;
    os << '\n';
    for (size_t i = 0; i < children.size(); ++i) {
        print_node(*children[i], "", i + 1 == children.size());
    }
}

SemanticAnalyzer::SemanticAnalyzer() {
    initialize_predefined();
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::analyze(const std::shared_ptr<ParseTreeNode>& root) {
    initialize_predefined();
    errors.clear();
    decorated_root.reset();
    decorated_root = visit_program(root);
    return decorated_root;
}

const std::vector<std::string>& SemanticAnalyzer::get_errors() const {
    return errors;
}

std::string SemanticAnalyzer::lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool SemanticAnalyzer::is_node(const std::shared_ptr<ParseTreeNode>& node, const std::string& name) {
    return node && node->name == name;
}

bool SemanticAnalyzer::terminal_is(const std::shared_ptr<ParseTreeNode>& node, const std::string& token_name) {
    return node && terminal_token(node) == token_name;
}

std::string SemanticAnalyzer::terminal_token(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return "";
    const auto pos = node->name.find('(');
    return pos == std::string::npos ? node->name : node->name.substr(0, pos);
}

std::string SemanticAnalyzer::terminal_value(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return "";
    const auto open = node->name.find('(');
    const auto close = node->name.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) {
        return "";
    }
    return node->name.substr(open + 1, close - open - 1);
}

std::shared_ptr<ParseTreeNode> SemanticAnalyzer::first_child(const std::shared_ptr<ParseTreeNode>& node,
                                                             const std::string& name) {
    if (!node) return nullptr;
    for (const auto& child : node->children) {
        if (child && child->name == name) return child;
    }
    return nullptr;
}

void SemanticAnalyzer::initialize_predefined() {
    tab.clear();
    btab.clear();
    atab.clear();
    types.clear();
    scopes.clear();
    scope_last.clear();
    display.clear();
    next_address.clear();

    SemanticType none_type;
    none_type.kind = SemanticTypeKind::None;
    none_type.name = "none";
    types.push_back(none_type);

    SemanticType base_type;
    base_type.kind = SemanticTypeKind::Integer;
    base_type.name = "Integer";
    integer_type = add_type(base_type);
    base_type.kind = SemanticTypeKind::Real;
    base_type.name = "Real";
    real_type = add_type(base_type);
    base_type.kind = SemanticTypeKind::Char;
    base_type.name = "Char";
    char_type = add_type(base_type);
    base_type.kind = SemanticTypeKind::Boolean;
    base_type.name = "Boolean";
    boolean_type = add_type(base_type);
    base_type.kind = SemanticTypeKind::String;
    base_type.name = "String";
    string_type = add_type(base_type);

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
        raw_add_symbol(word, "reserved", 0, 0, 1, 0, 0);
    }

    raw_add_symbol("Integer", "type", integer_type, 0, 1, 0, 0);
    raw_add_symbol("Real", "type", real_type, 0, 1, 0, 0);
    raw_add_symbol("Char", "type", char_type, 0, 1, 0, 0);
    raw_add_symbol("Boolean", "type", boolean_type, 0, 1, 0, 0);
    raw_add_symbol("String", "type", string_type, 0, 1, 0, 0);
    raw_add_symbol("True", "constant", boolean_type, 0, 1, 0, 1, true, "true");
    raw_add_symbol("False", "constant", boolean_type, 0, 1, 0, 0, true, "false");
    raw_add_symbol("readln", "procedure", 0, 0, 1, 0, 0);
    raw_add_symbol("writeln", "procedure", 0, 0, 1, 0, 0);
}

int SemanticAnalyzer::add_type(const SemanticType& type) {
    types.push_back(type);
    return static_cast<int>(types.size()) - 1;
}

std::string SemanticAnalyzer::type_name(int type_index) const {
    if (type_index > 0 && static_cast<size_t>(type_index) < types.size()) {
        return types[static_cast<size_t>(type_index)].name;
    }
    return "unknown";
}

int SemanticAnalyzer::type_size(int type_index) const {
    if (type_index <= 0 || static_cast<size_t>(type_index) >= types.size()) return 1;
    const auto& type = types[static_cast<size_t>(type_index)];
    if (type.kind == SemanticTypeKind::Real) return 2;
    if (type.kind == SemanticTypeKind::Array && type.ref >= 0 &&
        static_cast<size_t>(type.ref) < atab.size()) {
        return atab[static_cast<size_t>(type.ref)].size;
    }
    return 1;
}

int SemanticAnalyzer::raw_add_symbol(const std::string& id, const std::string& obj, int type,
                                     int ref, int nrm, int lev, int adr,
                                     bool initialized, const std::string& value) {
    const int index = static_cast<int>(tab.size());
    const int link = scope_last.empty() ? 0 : scope_last.back();
    tab.push_back({id, obj, type, ref, nrm, lev, adr, link, initialized, value});

    if (!scopes.empty()) {
        scopes.back()[lowercase(id)] = index;
        scope_last.back() = index;
        if (!display.empty() && display.back() >= 0 && static_cast<size_t>(display.back()) < btab.size()) {
            btab[static_cast<size_t>(display.back())].last = index;
        }
    }
    return index;
}

int SemanticAnalyzer::add_symbol(const std::string& id, const std::string& obj, int type,
                                 int ref, int nrm, bool initialized,
                                 const std::string& value) {
    if (lookup_current_scope(id) != -1) {
        semantic_error("Identifier '" + id + "' is already declared in the current scope");
        return lookup_current_scope(id);
    }

    const int adr = next_address.empty() ? 0 : next_address.back();
    if (obj == "variable") {
        next_address.back() += type_size(type);
        if (!display.empty() && static_cast<size_t>(display.back()) < btab.size()) {
            btab[static_cast<size_t>(display.back())].vsze += type_size(type);
        }
    }

    return raw_add_symbol(id, obj, type, ref, nrm,
                          static_cast<int>(scopes.size()) - 1, adr, initialized, value);
}

int SemanticAnalyzer::lookup(const std::string& id) const {
    const std::string key = lowercase(id);
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(key);
        if (found != it->end()) return found->second;
    }
    return -1;
}

int SemanticAnalyzer::lookup_current_scope(const std::string& id) const {
    if (scopes.empty()) return -1;
    const auto found = scopes.back().find(lowercase(id));
    return found == scopes.back().end() ? -1 : found->second;
}

void SemanticAnalyzer::push_scope(int block_index) {
    scopes.push_back({});
    scope_last.push_back(0);
    display.push_back(block_index);
    next_address.push_back(0);
}

void SemanticAnalyzer::pop_scope() {
    if (!scopes.empty()) scopes.pop_back();
    if (!scope_last.empty()) scope_last.pop_back();
    if (!display.empty()) display.pop_back();
    if (!next_address.empty()) next_address.pop_back();
}

void SemanticAnalyzer::semantic_error(const std::string& message) {
    errors.push_back("Semantic error: " + message);
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_program(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Program");
    if (!node) {
        semantic_error("Parse tree root is empty");
        return ast;
    }

    auto header = first_child(node, "<program-header>");
    std::string program_name = "<anonymous>";
    if (header) {
        for (const auto& child : header->children) {
            if (terminal_is(child, "ident")) {
                program_name = terminal_value(child);
                break;
            }
        }
    }

    const int program_index = add_symbol(program_name, "program", 0, 0, 1, true);
    ast->label = "Program(" + program_name + ")";
    ast->tab_index = program_index;
    ast->block_index = 0;

    ast->add_child(visit_declaration_part(first_child(node, "<declaration-part>")));
    ast->add_child(visit_compound_statement(first_child(node, "<compound-statement>")));
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_declaration_part(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Declarations");
    if (!node) return ast;

    for (const auto& child : node->children) {
        if (is_node(child, "<const-declaration>")) ast->add_child(visit_const_declaration(child));
        else if (is_node(child, "<type-declaration>")) ast->add_child(visit_type_declaration(child));
        else if (is_node(child, "<var-declaration>")) ast->add_child(visit_var_declaration(child));
        else if (is_node(child, "<subprogram-declaration>")) ast->add_child(visit_subprogram_declaration(child));
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_const_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("ConstDecls");
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!terminal_is(node->children[i], "ident")) continue;
        const std::string id = terminal_value(node->children[i]);
        auto constant = (i + 2 < node->children.size()) ? node->children[i + 2] : nullptr;
        ValueInfo value = eval_constant(constant);
        const int idx = add_symbol(id, "constant", value.type, 0, 1, true, value.string_value);
        auto child = std::make_shared<DecoratedAstNode>("ConstDecl(" + id + ")");
        child->type = value.type;
        child->tab_index = idx;
        child->level = static_cast<int>(scopes.size()) - 1;
        ast->add_child(child);
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_type_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("TypeDecls");
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!terminal_is(node->children[i], "ident")) continue;
        const std::string id = terminal_value(node->children[i]);
        auto type_node = (i + 2 < node->children.size()) ? node->children[i + 2] : nullptr;
        int resolved = resolve_type_node(type_node);
        if (resolved > 0 && static_cast<size_t>(resolved) < types.size()) {
            auto& info = types[static_cast<size_t>(resolved)];
            if (info.anonymous) {
                info.name = id;
                info.anonymous = false;
            }
        }
        const int idx = add_symbol(id, "type", resolved, 0, 1, true);
        auto child = std::make_shared<DecoratedAstNode>("TypeDecl(" + id + ")");
        child->type = resolved;
        child->tab_index = idx;
        child->level = static_cast<int>(scopes.size()) - 1;
        ast->add_child(child);
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_var_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("VarDecls");
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!is_node(node->children[i], "<identifier-list>")) continue;
        std::vector<std::string> ids = collect_identifier_list(node->children[i]);
        auto type_node = (i + 2 < node->children.size()) ? node->children[i + 2] : nullptr;
        int resolved = resolve_type_node(type_node);
        for (const auto& id : ids) {
            const int idx = add_symbol(id, "variable", resolved, 0, 1, false);
            auto child = std::make_shared<DecoratedAstNode>("VarDecl(" + id + ")");
            child->type = resolved;
            child->tab_index = idx;
            child->level = static_cast<int>(scopes.size()) - 1;
            ast->add_child(child);
        }
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_subprogram_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("SubprogramDecl");
    if (!node || node->children.empty()) return ast;

    auto decl = node->children.front();
    const bool is_function = is_node(decl, "<function-declaration>");
    const std::string obj = is_function ? "function" : "procedure";
    std::string id = "<subprogram>";
    int result_type = 0;

    for (const auto& child : decl->children) {
        if (terminal_is(child, "ident")) {
            if (id == "<subprogram>") id = terminal_value(child);
            else if (is_function && result_type == 0) {
                const int type_idx = lookup(terminal_value(child));
                if (type_idx >= 0 && tab[static_cast<size_t>(type_idx)].obj == "type") {
                    result_type = tab[static_cast<size_t>(type_idx)].type;
                }
            }
        }
    }

    const int block_index = static_cast<int>(btab.size());
    btab.push_back({});
    const int sub_idx = add_symbol(id, obj, result_type, block_index, 1, true);
    ast->label = obj + "(" + id + ")";
    ast->tab_index = sub_idx;
    ast->block_index = block_index;

    push_scope(block_index);
    for (const auto& child : decl->children) {
        if (is_node(child, "<formal-parameter-list>")) {
            for (const auto& group : child->children) {
                if (!is_node(group, "<parameter-group>")) continue;
                auto ids = collect_identifier_list(first_child(group, "<identifier-list>"));
                int param_type = 0;
                for (const auto& item : group->children) {
                    if (terminal_is(item, "ident")) {
                        const int type_idx = lookup(terminal_value(item));
                        if (type_idx >= 0 && tab[static_cast<size_t>(type_idx)].obj == "type") {
                            param_type = tab[static_cast<size_t>(type_idx)].type;
                        }
                    } else if (is_node(item, "<array-type>")) {
                        param_type = resolve_array_type(item);
                    }
                }
                for (const auto& param : ids) {
                    const int idx = add_symbol(param, "parameter", param_type, 0, 1, true);
                    btab[static_cast<size_t>(block_index)].lpar = idx;
                    btab[static_cast<size_t>(block_index)].psze += type_size(param_type);
                }
            }
        } else if (is_node(child, "<block>")) {
            ast->add_child(visit_block(child, false));
        }
    }
    pop_scope();
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_block(const std::shared_ptr<ParseTreeNode>& node,
                                                                bool creates_scope) {
    int block_index = display.empty() ? 0 : display.back();
    if (creates_scope) {
        block_index = static_cast<int>(btab.size());
        btab.push_back({});
        push_scope(block_index);
    }
    auto ast = std::make_shared<DecoratedAstNode>("Block");
    ast->block_index = block_index;
    ast->add_child(visit_declaration_part(first_child(node, "<declaration-part>")));
    ast->add_child(visit_compound_statement(first_child(node, "<compound-statement>")));
    if (creates_scope) pop_scope();
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_compound_statement(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Block");
    ast->block_index = display.empty() ? 0 : display.back();
    ast->add_child(visit_statement_list(first_child(node, "<statement-list>")));
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_statement_list(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Statements");
    if (!node) return ast;
    for (const auto& child : node->children) {
        if (is_node(child, "<statement>")) ast->add_child(visit_statement(child));
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_statement(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node || node->children.empty()) {
        return std::make_shared<DecoratedAstNode>("Empty");
    }
    const auto& child = node->children.front();
    if (is_node(child, "<assignment-statement>")) return visit_assignment(child);
    if (is_node(child, "<if-statement>")) return visit_if(child);
    if (is_node(child, "<while-statement>")) return visit_while(child);
    if (is_node(child, "<repeat-statement>")) return visit_repeat(child);
    if (is_node(child, "<for-statement>")) return visit_for(child);
    if (is_node(child, "<case-statement>")) return visit_case(child);
    if (is_node(child, "<procedure/function-call>")) return visit_call(child);
    if (is_node(child, "<compound-statement>")) return visit_compound_statement(child);
    if (is_node(child, "<variable>")) {
        ValueInfo value = eval_variable(child);
        auto ast = std::make_shared<DecoratedAstNode>("VariableUse");
        ast->type = value.type;
        ast->tab_index = value.tab_index;
        return ast;
    }
    return std::make_shared<DecoratedAstNode>("UnknownStatement");
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_assignment(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Assign");
    ValueInfo target = eval_variable(first_child(node, "<variable>"), true);
    ValueInfo value = eval_expression(first_child(node, "<expression>"));
    ast->type = target.type;
    ast->tab_index = target.tab_index;
    if (!assignment_compatible(target.type, value)) {
        semantic_error("Cannot assign " + type_name(value.type) + " to " + type_name(target.type));
    }
    if (target.tab_index >= 0 && static_cast<size_t>(target.tab_index) < tab.size()) {
        tab[static_cast<size_t>(target.tab_index)].initialized = true;
    }
    ast->add_child(std::make_shared<DecoratedAstNode>("target"));
    ast->children.back()->type = target.type;
    ast->children.back()->tab_index = target.tab_index;
    ast->add_child(std::make_shared<DecoratedAstNode>("value"));
    ast->children.back()->type = value.type;
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_if(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("If");
    auto expr = first_child(node, "<expression>");
    ValueInfo cond = eval_expression(expr);
    if (cond.type != boolean_type) semantic_error("if condition must be Boolean");
    ast->type = cond.type;
    for (const auto& child : node->children) {
        if (is_node(child, "<statement>")) ast->add_child(visit_statement(child));
    }
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_while(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("While");
    ValueInfo cond = eval_expression(first_child(node, "<expression>"));
    if (cond.type != boolean_type) semantic_error("while condition must be Boolean");
    ast->type = cond.type;
    ast->add_child(visit_compound_statement(first_child(node, "<compound-statement>")));
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_repeat(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Repeat");
    ast->add_child(visit_statement_list(first_child(node, "<statement-list>")));
    ValueInfo cond;
    for (const auto& child : node->children) {
        if (is_node(child, "<expression>")) cond = eval_expression(child);
    }
    if (cond.type != boolean_type) semantic_error("repeat-until condition must be Boolean");
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_for(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("For");
    std::string iterator;
    std::vector<std::shared_ptr<ParseTreeNode>> expressions;
    for (const auto& child : node->children) {
        if (terminal_is(child, "ident") && iterator.empty()) iterator = terminal_value(child);
        if (is_node(child, "<expression>")) expressions.push_back(child);
    }
    const int idx = lookup(iterator);
    if (idx < 0) semantic_error("for iterator '" + iterator + "' is not declared");
    else if (tab[static_cast<size_t>(idx)].type != integer_type && tab[static_cast<size_t>(idx)].type != char_type) {
        semantic_error("for iterator '" + iterator + "' must be Integer or Char");
    }
    for (const auto& expr : expressions) {
        ValueInfo value = eval_expression(expr);
        if (idx >= 0 && !assignment_compatible(tab[static_cast<size_t>(idx)].type, value)) {
            semantic_error("for bound type is incompatible with iterator '" + iterator + "'");
        }
    }
    ast->tab_index = idx;
    ast->add_child(visit_compound_statement(first_child(node, "<compound-statement>")));
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_case(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = std::make_shared<DecoratedAstNode>("Case");
    ValueInfo selector = eval_expression(first_child(node, "<expression>"));
    auto block = first_child(node, "<case-block>");
    for (const auto& child : block ? block->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (is_node(child, "<constant>")) {
            ValueInfo label = eval_constant(child);
            if (!compatible(selector.type, label.type)) {
                semantic_error("case label type " + type_name(label.type) +
                               " is incompatible with selector " + type_name(selector.type));
            }
        } else if (is_node(child, "<statement>")) {
            ast->add_child(visit_statement(child));
        }
    }
    ast->type = selector.type;
    return ast;
}

std::shared_ptr<DecoratedAstNode> SemanticAnalyzer::visit_call(const std::shared_ptr<ParseTreeNode>& node,
                                                               ValueInfo* out) {
    std::string id;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (terminal_is(child, "ident")) {
            id = terminal_value(child);
            break;
        }
    }
    const int idx = lookup(id);
    if (idx < 0) {
        semantic_error("Procedure/function '" + id + "' is not declared");
    } else if (tab[static_cast<size_t>(idx)].obj != "procedure" &&
               tab[static_cast<size_t>(idx)].obj != "function") {
        semantic_error("Identifier '" + id + "' is not callable");
    }

    auto ast = std::make_shared<DecoratedAstNode>("Call(" + id + ")");
    ast->tab_index = idx;
    if (idx >= 0) ast->type = tab[static_cast<size_t>(idx)].type;
    auto params = first_child(node, "<parameter-list>");
    if (params) {
        for (const auto& child : params->children) {
            if (is_node(child, "<expression>")) {
                ValueInfo arg = eval_expression(child);
                auto arg_ast = std::make_shared<DecoratedAstNode>("arg");
                arg_ast->type = arg.type;
                ast->add_child(arg_ast);
            }
        }
    }
    if (out) {
        out->type = ast->type;
        out->tab_index = idx;
        out->initialized = true;
    }
    return ast;
}

int SemanticAnalyzer::resolve_type_node(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return 0;
    if (is_node(node, "<type>") && !node->children.empty()) return resolve_type_node(node->children.front());
    if (terminal_is(node, "ident")) {
        const std::string id = terminal_value(node);
        const int idx = lookup(id);
        if (idx < 0 || tab[static_cast<size_t>(idx)].obj != "type") {
            semantic_error("Type '" + id + "' is not declared");
            return 0;
        }
        return tab[static_cast<size_t>(idx)].type;
    }
    if (is_node(node, "<array-type>")) return resolve_array_type(node);
    if (is_node(node, "<range>")) return resolve_range_type(node);
    if (is_node(node, "<enumerated>")) return resolve_enumerated_type(node);
    if (is_node(node, "<record-type>")) return resolve_record_type(node);
    return 0;
}

int SemanticAnalyzer::resolve_array_type(const std::shared_ptr<ParseTreeNode>& node) {
    int index_type = 0;
    int low = 0;
    int high = 0;
    int element_type = 0;
    bool after_of = false;

    for (const auto& child : node->children) {
        if (terminal_is(child, "ofsy")) {
            after_of = true;
        } else if (!after_of && is_node(child, "<range>")) {
            index_type = resolve_range_type(child);
            if (index_type > 0) {
                low = types[static_cast<size_t>(index_type)].low;
                high = types[static_cast<size_t>(index_type)].high;
            }
        } else if (!after_of && terminal_is(child, "ident")) {
            const int idx = lookup(terminal_value(child));
            if (idx < 0 || tab[static_cast<size_t>(idx)].obj != "type") {
                semantic_error("Array index type '" + terminal_value(child) + "' is not declared");
        } else {
            index_type = tab[static_cast<size_t>(idx)].type;
            if (index_type > 0 && static_cast<size_t>(index_type) < types.size() &&
                types[static_cast<size_t>(index_type)].kind == SemanticTypeKind::Subrange) {
                low = types[static_cast<size_t>(index_type)].low;
                high = types[static_cast<size_t>(index_type)].high;
            }
        }
        } else if (after_of && (is_node(child, "<type>") || terminal_is(child, "ident"))) {
            element_type = resolve_type_node(child);
        }
    }

    if (!simple_non_real(index_type)) {
        semantic_error("Array index type must be a simple non-Real type");
    }

    const int elem_size = type_size(element_type);
    const int count = high >= low ? high - low + 1 : 0;
    const int array_ref = static_cast<int>(atab.size());
    atab.push_back({index_type, element_type, 0, low, high, elem_size, count * elem_size});

    SemanticType type;
    type.kind = SemanticTypeKind::Array;
    type.name = "anonymous_array";
    type.base_type = element_type;
    type.ref = array_ref;
    type.anonymous = true;
    return add_type(type);
}

int SemanticAnalyzer::resolve_range_type(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<ValueInfo> constants;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (is_node(child, "<constant>")) constants.push_back(eval_constant(child));
    }
    if (constants.size() != 2) return 0;
    if (constants[0].type != constants[1].type) {
        semantic_error("Range bounds must have the same type");
    }
    if (constants[0].type == real_type) {
        semantic_error("Subrange cannot use Real bounds");
    }

    int low = 0;
    int high = 0;
    if (constants[0].type == integer_type) {
        low = constants[0].int_value;
        high = constants[1].int_value;
    } else if (constants[0].type == char_type) {
        low = constants[0].char_value;
        high = constants[1].char_value;
    }
    if (low > high) semantic_error("Range lower bound is greater than upper bound");

    SemanticType type;
    type.kind = SemanticTypeKind::Subrange;
    type.name = "subrange(" + type_name(constants[0].type) + ")";
    type.base_type = constants[0].type;
    type.low = low;
    type.high = high;
    type.anonymous = true;
    return add_type(type);
}

int SemanticAnalyzer::resolve_enumerated_type(const std::shared_ptr<ParseTreeNode>& node) {
    int common_type = 0;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (!terminal_is(child, "ident")) continue;
        const std::string id = terminal_value(child);
        const int idx = lookup(id);
        if (idx < 0) {
            semantic_error("Enumerated identifier '" + id + "' must be declared before the enumerated type");
            continue;
        }
        const int current_type = tab[static_cast<size_t>(idx)].type;
        if (common_type == 0) common_type = current_type;
        else if (!compatible(common_type, current_type)) {
            semantic_error("Enumerated identifiers must have compatible types");
        }
    }
    SemanticType type;
    type.kind = SemanticTypeKind::Enumerated;
    type.name = "enumerated";
    type.base_type = common_type;
    type.anonymous = true;
    return add_type(type);
}

int SemanticAnalyzer::resolve_record_type(const std::shared_ptr<ParseTreeNode>& node) {
    const int block_index = static_cast<int>(btab.size());
    btab.push_back({});
    push_scope(block_index);

    SemanticType type;
    type.kind = SemanticTypeKind::Record;
    type.name = "anonymous_record";
    type.ref = block_index;
    type.anonymous = true;

    auto fields = first_child(first_child(node, "<field-list>"), "<field-part>");
    std::function<void(const std::shared_ptr<ParseTreeNode>&)> visit_fields =
        [&](const std::shared_ptr<ParseTreeNode>& field_list) {
            if (!field_list) return;
            for (const auto& field_part : field_list->children) {
                if (!is_node(field_part, "<field-part>")) continue;
                auto ids = collect_identifier_list(first_child(field_part, "<identifier-list>"));
                int field_type = resolve_type_node(first_child(field_part, "<type>"));
                for (const auto& id : ids) {
                    const int idx = add_symbol(id, "field", field_type, 0, 1, true);
                    type.fields[lowercase(id)] = idx;
                }
            }
        };
    (void)fields;
    visit_fields(first_child(node, "<field-list>"));
    pop_scope();

    return add_type(type);
}

std::vector<std::string> SemanticAnalyzer::collect_identifier_list(const std::shared_ptr<ParseTreeNode>& node) const {
    std::vector<std::string> ids;
    if (!node) return ids;
    for (const auto& child : node->children) {
        if (terminal_is(child, "ident")) ids.push_back(terminal_value(child));
    }
    return ids;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_constant(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    if (!node) return result;

    int sign = 1;
    for (const auto& child : node->children) {
        if (terminal_is(child, "minus")) sign = -1;
        else if (terminal_is(child, "plus")) sign = 1;
        else {
            result = value_from_terminal(child);
        }
    }
    if (result.type == integer_type) result.int_value *= sign;
    if (result.type == real_type) result.real_value *= sign;
    result.is_constant = true;
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_expression(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    std::vector<std::shared_ptr<ParseTreeNode>> simple_exprs;
    bool has_relation = false;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (is_node(child, "<simple-expression>")) simple_exprs.push_back(child);
        if (is_node(child, "<relational-operator>")) has_relation = true;
    }
    if (simple_exprs.empty()) return result;
    result = eval_simple_expression(simple_exprs[0]);
    if (has_relation && simple_exprs.size() == 2) {
        ValueInfo right = eval_simple_expression(simple_exprs[1]);
        if (!compatible(result.type, right.type)) {
            semantic_error("Relational operands are incompatible: " + type_name(result.type) +
                           " and " + type_name(right.type));
        }
        result.type = boolean_type;
        result.is_constant = false;
    }
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_simple_expression(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    std::vector<ValueInfo> terms;
    std::vector<std::string> ops;
    int sign = 1;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (terminal_is(child, "minus")) sign = -1;
        else if (terminal_is(child, "plus")) sign = 1;
        else if (is_node(child, "<term>")) terms.push_back(eval_term(child));
        else if (is_node(child, "<additive-operator>") && !child->children.empty()) {
            ops.push_back(terminal_token(child->children.front()));
        }
    }
    if (terms.empty()) return result;
    result = terms[0];
    if (result.type == integer_type) result.int_value *= sign;
    if (result.type == real_type) result.real_value *= sign;

    for (size_t i = 1; i < terms.size(); ++i) {
        const std::string op = i - 1 < ops.size() ? ops[i - 1] : "";
        if (op == "orsy") {
            if (result.type != boolean_type || terms[i].type != boolean_type) {
                semantic_error("Operator or requires Boolean operands");
            }
            result.type = boolean_type;
        } else {
            if (!numeric(result.type) || !numeric(terms[i].type)) {
                semantic_error("Operator " + op + " requires numeric operands");
                result.type = 0;
            } else {
                result.type = (result.type == real_type || terms[i].type == real_type) ? real_type : integer_type;
            }
        }
        result.is_constant = false;
    }
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_term(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    std::vector<ValueInfo> factors;
    std::vector<std::string> ops;
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (is_node(child, "<factor>")) factors.push_back(eval_factor(child));
        else if (is_node(child, "<multiplicative-operator>") && !child->children.empty()) {
            ops.push_back(terminal_token(child->children.front()));
        }
    }
    if (factors.empty()) return result;
    result = factors[0];
    for (size_t i = 1; i < factors.size(); ++i) {
        const std::string op = i - 1 < ops.size() ? ops[i - 1] : "";
        if (op == "andsy") {
            if (result.type != boolean_type || factors[i].type != boolean_type) {
                semantic_error("Operator and requires Boolean operands");
            }
            result.type = boolean_type;
        } else if (op == "idiv" || op == "imod") {
            if (result.type != integer_type || factors[i].type != integer_type) {
                semantic_error("Operator " + op + " requires Integer operands");
            }
            result.type = integer_type;
        } else {
            if (!numeric(result.type) || !numeric(factors[i].type)) {
                semantic_error("Operator " + op + " requires numeric operands");
                result.type = 0;
            } else {
                result.type = (op == "rdiv" || result.type == real_type || factors[i].type == real_type)
                                  ? real_type
                                  : integer_type;
            }
        }
        result.is_constant = false;
    }
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_factor(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    if (!node) return result;
    bool negated = false;
    for (const auto& child : node->children) {
        if (terminal_is(child, "notsy")) negated = true;
        else if (is_node(child, "<variable>")) result = eval_variable(child);
        else if (is_node(child, "<procedure/function-call>")) visit_call(child, &result);
        else if (is_node(child, "<expression>")) result = eval_expression(child);
        else if (is_node(child, "<factor>")) result = eval_factor(child);
        else if (terminal_is(child, "intcon") || terminal_is(child, "realcon") ||
                 terminal_is(child, "charcon") || terminal_is(child, "string")) {
            result = value_from_terminal(child);
        }
    }
    if (negated) {
        if (result.type != boolean_type) semantic_error("Operator not requires a Boolean operand");
        result.type = boolean_type;
    }
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::eval_variable(const std::shared_ptr<ParseTreeNode>& node,
                                                            bool as_assignment_target) {
    ValueInfo result;
    if (!node) return result;

    std::string id;
    size_t i = 0;
    for (; i < node->children.size(); ++i) {
        if (terminal_is(node->children[i], "ident")) {
            id = terminal_value(node->children[i]);
            break;
        }
    }

    const int idx = lookup(id);
    if (idx < 0) {
        semantic_error("Identifier '" + id + "' is not declared");
        result.type = 0;
        return result;
    }

    result.type = tab[static_cast<size_t>(idx)].type;
    result.tab_index = idx;
    result.initialized = tab[static_cast<size_t>(idx)].initialized;
    if (!as_assignment_target && !result.initialized && tab[static_cast<size_t>(idx)].obj == "variable") {
        semantic_error("Variable '" + id + "' may be used before initialization");
    }

    for (; i < node->children.size(); ++i) {
        const auto& component = node->children[i];
        if (!is_node(component, "<component-variable>")) continue;
        if (component->children.empty()) continue;
        if (terminal_is(component->children.front(), "lbrack")) {
            if (result.type <= 0 || static_cast<size_t>(result.type) >= types.size() ||
                types[static_cast<size_t>(result.type)].kind != SemanticTypeKind::Array) {
                semantic_error("Indexed access requires an Array type");
                result.type = 0;
                continue;
            }
            const int array_ref = types[static_cast<size_t>(result.type)].ref;
            if (array_ref >= 0 && static_cast<size_t>(array_ref) < atab.size()) {
                result.type = atab[static_cast<size_t>(array_ref)].etyp;
            }
        } else if (terminal_is(component->children.front(), "period")) {
            if (result.type <= 0 || static_cast<size_t>(result.type) >= types.size() ||
                types[static_cast<size_t>(result.type)].kind != SemanticTypeKind::Record) {
                semantic_error("Field access requires a Record type");
                result.type = 0;
                continue;
            }
            std::string field;
            for (const auto& c : component->children) {
                if (terminal_is(c, "ident")) field = terminal_value(c);
            }
            const auto found = types[static_cast<size_t>(result.type)].fields.find(lowercase(field));
            if (found == types[static_cast<size_t>(result.type)].fields.end()) {
                semantic_error("Record field '" + field + "' is not declared");
                result.type = 0;
            } else {
                result.tab_index = found->second;
                result.type = tab[static_cast<size_t>(found->second)].type;
            }
        }
    }
    return result;
}

SemanticAnalyzer::ValueInfo SemanticAnalyzer::value_from_terminal(const std::shared_ptr<ParseTreeNode>& node) {
    ValueInfo result;
    const std::string token = terminal_token(node);
    const std::string value = terminal_value(node);
    result.is_constant = true;
    result.initialized = true;
    result.string_value = value;

    try {
        if (token == "intcon") {
            result.type = integer_type;
            result.int_value = std::stoi(value);
        } else if (token == "realcon") {
            result.type = real_type;
            result.real_value = std::stod(value);
        } else if (token == "charcon") {
            result.type = char_type;
            result.char_value = value.size() >= 3 ? value[1] : '\0';
        } else if (token == "string") {
            result.type = string_type;
            result.length = static_cast<int>(value.size());
        } else if (token == "ident") {
            const int idx = lookup(value);
            if (idx < 0) {
                semantic_error("Identifier '" + value + "' is not declared");
            } else {
                const auto& entry = tab[static_cast<size_t>(idx)];
                result.type = entry.type;
                result.tab_index = idx;
                result.initialized = entry.initialized;
                result.is_constant = entry.obj == "constant";
                result.string_value = entry.value;
                const std::string lowered_value = lowercase(value);
                const std::string lowered_text = lowercase(result.string_value);
                if (result.type == integer_type && !result.string_value.empty()) {
                    result.int_value = std::stoi(result.string_value);
                } else if (result.type == real_type && !result.string_value.empty()) {
                    result.real_value = std::stod(result.string_value);
                } else if (result.type == char_type && result.string_value.size() >= 3) {
                    result.char_value = result.string_value[1];
                } else if (result.type == boolean_type) {
                    result.bool_value = lowered_value == "true" || lowered_text == "true";
                }
            }
        }
    } catch (const std::exception&) {
        semantic_error("Invalid literal value '" + value + "'");
    }
    return result;
}

bool SemanticAnalyzer::compatible(int left_type, int right_type) const {
    if (left_type == 0 || right_type == 0) return true;
    if (left_type == right_type) return true;
    auto base = [&](int type) {
        if (type > 0 && static_cast<size_t>(type) < types.size() &&
            types[static_cast<size_t>(type)].kind == SemanticTypeKind::Subrange) {
            return types[static_cast<size_t>(type)].base_type;
        }
        return type;
    };
    return base(left_type) == base(right_type);
}

bool SemanticAnalyzer::assignment_compatible(int target_type, const ValueInfo& value) const {
    if (target_type == 0 || value.type == 0) return true;
    if (target_type == real_type && value.type == integer_type) return true;
    if (compatible(target_type, value.type)) {
        if (target_type > 0 && static_cast<size_t>(target_type) < types.size() &&
            types[static_cast<size_t>(target_type)].kind == SemanticTypeKind::Subrange &&
            value.is_constant) {
            const auto& type = types[static_cast<size_t>(target_type)];
            int scalar = 0;
            if (value.type == integer_type) scalar = value.int_value;
            else if (value.type == char_type) scalar = value.char_value;
            return scalar >= type.low && scalar <= type.high;
        }
        return true;
    }
    return false;
}

bool SemanticAnalyzer::numeric(int type) const {
    return type == integer_type || type == real_type ||
           (type > 0 && static_cast<size_t>(type) < types.size() &&
            types[static_cast<size_t>(type)].kind == SemanticTypeKind::Subrange &&
            types[static_cast<size_t>(type)].base_type == integer_type);
}

bool SemanticAnalyzer::simple_non_real(int type) const {
    if (type == integer_type || type == char_type || type == boolean_type) return true;
    if (type > 0 && static_cast<size_t>(type) < types.size()) {
        const auto& info = types[static_cast<size_t>(type)];
        return (info.kind == SemanticTypeKind::Subrange && info.base_type != real_type) ||
               info.kind == SemanticTypeKind::Enumerated;
    }
    return false;
}

void SemanticAnalyzer::print_report(std::ostream& os) const {
    os << "\n=== Decorated AST ===\n";
    if (decorated_root) decorated_root->print(os, types);

    os << "\n=== Symbol Table: tab ===\n";
    os << "idx\tidentifier\tobj\ttype\tref\tnrm\tlev\tadr\tlink\tinit\tvalue\n";
    for (size_t i = 0; i < tab.size(); ++i) {
        const auto& entry = tab[i];
        os << i << '\t' << entry.identifier << '\t' << entry.obj << '\t'
           << type_name(entry.type) << '\t' << entry.ref << '\t' << entry.nrm << '\t'
           << entry.lev << '\t' << entry.adr << '\t' << entry.link << '\t'
           << (entry.initialized ? "yes" : "no") << '\t' << entry.value << '\n';
    }

    os << "\n=== Symbol Table: btab ===\n";
    os << "idx\tlast\tlpar\tpsze\tvsze\n";
    for (size_t i = 0; i < btab.size(); ++i) {
        os << i << '\t' << btab[i].last << '\t' << btab[i].lpar << '\t'
           << btab[i].psze << '\t' << btab[i].vsze << '\n';
    }

    os << "\n=== Symbol Table: atab ===\n";
    os << "idx\txtyp\tetyp\teref\tlow\thigh\telsz\tsize\n";
    for (size_t i = 0; i < atab.size(); ++i) {
        os << i << '\t' << type_name(atab[i].xtyp) << '\t'
           << type_name(atab[i].etyp) << '\t' << atab[i].eref << '\t'
           << atab[i].low << '\t' << atab[i].high << '\t'
           << atab[i].elsz << '\t' << atab[i].size << '\n';
    }

    os << "\n=== Semantic Errors ===\n";
    if (errors.empty()) {
        os << "No semantic errors.\n";
    } else {
        for (const auto& error : errors) os << error << '\n';
    }
}
