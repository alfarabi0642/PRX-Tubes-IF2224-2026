#include "semantic_analyzer.hpp"

#include <algorithm>

namespace semantic {

SemanticResult::SemanticResult() {
    symbols.initialize_predefined(types);
}

bool SemanticResult::ok() const {
    return !has_errors(diagnostics);
}

SemanticResult SemanticAnalyzer::analyze(const AstNodePtr& ast_root) {
    SemanticResult semantic_result;
    result = &semantic_result;
    semantic_result.decorated_ast = visit_program(ast_root);
    result = nullptr;
    return semantic_result;
}

void SemanticAnalyzer::semantic_error(const AstNodePtr& node, const std::string& message) {
    result->diagnostics.push_back({
        DiagnosticSeverity::Error,
        node ? node->location : SourceLocation{},
        "Semantic error: " + message
    });
}

int SemanticAnalyzer::type_size(int type_id) const {
    if (!result) return 1;
    const auto& types = result->types.entries();
    if (type_id <= 0 || static_cast<size_t>(type_id) >= types.size()) return 1;
    const auto& type = types[static_cast<size_t>(type_id)];
    if (type.kind == TypeKind::Real) return 2;
    if (type.kind == TypeKind::Array && type.ref >= 0 &&
        static_cast<size_t>(type.ref) < result->symbols.atab_entries().size()) {
        return result->symbols.atab_entries()[static_cast<size_t>(type.ref)].size;
    }
    return 1;
}

int SemanticAnalyzer::add_symbol_checked(const AstNodePtr& node,
                                         const std::string& id,
                                         SymbolObject obj,
                                         int type,
                                         int ref,
                                         int nrm,
                                         bool initialized,
                                         const std::string& value) {
    const int existing = result->symbols.lookup_current_scope(id);
    if (existing != -1) {
        semantic_error(node, "Identifier '" + id + "' is already declared in the current scope");
        return existing;
    }
    return result->symbols.add_symbol(id, obj, type, ref, nrm, initialized, value, type_size(type));
}

AstNodePtr SemanticAnalyzer::visit_program(const AstNodePtr& node) {
    if (!node) {
        semantic_error(nullptr, "AST root is empty");
        return make_ast(AstKind::Program);
    }

    const int program_index = add_symbol_checked(node, node->name, SymbolObject::Program, 0, 0, 1, true);
    node->annotation.tab_index = program_index;
    node->annotation.block_index = result->symbols.current_block_index();
    node->annotation.lexical_level = result->symbols.current_level();

    for (const auto& child : node->children) {
        if (child && child->kind == AstKind::DeclarationPart) visit_declaration_part(child);
        else if (child && child->kind == AstKind::CompoundStatement) visit_compound_statement(child);
    }
    return node;
}

AstNodePtr SemanticAnalyzer::visit_declaration_part(const AstNodePtr& node) {
    if (!node) return nullptr;
    for (const auto& child : node->children) {
        if (!child) continue;
        switch (child->kind) {
            case AstKind::ConstDecl: visit_const_decl(child); break;
            case AstKind::TypeDecl: visit_type_decl(child); break;
            case AstKind::VarDecl: visit_var_decl(child); break;
            case AstKind::ProcedureDecl:
            case AstKind::FunctionDecl:
                visit_subprogram_decl(child);
                break;
            default:
                break;
        }
    }
    return node;
}

AstNodePtr SemanticAnalyzer::visit_compound_statement(const AstNodePtr& node) {
    if (!node) return nullptr;
    node->annotation.block_index = result->symbols.current_block_index();
    node->annotation.lexical_level = result->symbols.current_level();
    for (const auto& child : node->children) {
        visit_statement(child);
    }
    return node;
}

AstNodePtr SemanticAnalyzer::visit_statement(const AstNodePtr& node) {
    if (!node) return nullptr;
    switch (node->kind) {
        case AstKind::EmptyStatement: return node;
        case AstKind::AssignStatement: return visit_assignment(node);
        case AstKind::IfStatement: return visit_if(node);
        case AstKind::WhileStatement: return visit_while(node);
        case AstKind::RepeatStatement: return visit_repeat(node);
        case AstKind::ForStatement: return visit_for(node);
        case AstKind::CaseStatement: return visit_case(node);
        case AstKind::Call: return visit_call(node);
        case AstKind::CompoundStatement: return visit_compound_statement(node);
        case AstKind::Variable: {
            ValueInfo value = eval_variable(node);
            node->annotation.type_id = value.type;
            node->annotation.tab_index = value.tab_index;
            return node;
        }
        default:
            return node;
    }
}

} // namespace semantic
