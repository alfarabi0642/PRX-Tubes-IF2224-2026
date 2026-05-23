#pragma once

#include <string>
#include <vector>
#include "ast.hpp"
#include "diagnostic.hpp"
#include "symbol_table.hpp"
#include "types.hpp"

namespace semantic {

// Expression value metadata
struct ValueInfo {
    int type = 0;
    bool is_constant = false;
    bool initialized = false;
    int tab_index = -1;
    int base_tab_index = -1;
    int int_value = 0;
    double real_value = 0.0;
    char char_value = '\0';
    bool bool_value = false;
    int length = 0;
    std::string string_value;
};

// Semantic pass result
struct SemanticResult {
    AstNodePtr decorated_ast;
    TypeRegistry types;
    SymbolTable symbols;
    std::vector<Diagnostic> diagnostics;

    SemanticResult();
    bool ok() const;
};

// AST semantic visitor
class SemanticAnalyzer {
public:
    SemanticResult analyze(const AstNodePtr& ast_root);

private:
    SemanticResult* result = nullptr;

    void semantic_error(const AstNodePtr& node, const std::string& message);
    int type_size(int type_id) const;
    int type_ref(int type_id) const;
    std::vector<int> parameter_types_for(const TabEntry& entry) const;
    void validate_user_call_arguments(const AstNodePtr& node,
                                      const TabEntry& entry,
                                      const std::vector<ValueInfo>& arguments);
    bool validate_builtin_call(const AstNodePtr& node,
                               const TabEntry& entry,
                               ValueInfo* out);
    bool is_assignable_storage(const ValueInfo& target,
                               bool allow_current_function) const;
    bool is_scalar_or_string_type(int type_id) const;
    void mark_target_initialized(const ValueInfo& target);
    int add_symbol_checked(const AstNodePtr& node,
                           const std::string& id,
                           SymbolObject obj,
                           int type,
                           int ref = 0,
                           int nrm = 1,
                           bool initialized = false,
                           const std::string& value = "");

    AstNodePtr visit_program(const AstNodePtr& node);
    AstNodePtr visit_declaration_part(const AstNodePtr& node);
    AstNodePtr visit_const_decl(const AstNodePtr& node);
    AstNodePtr visit_type_decl(const AstNodePtr& node);
    AstNodePtr visit_var_decl(const AstNodePtr& node);
    AstNodePtr visit_subprogram_decl(const AstNodePtr& node);
    AstNodePtr visit_compound_statement(const AstNodePtr& node);
    AstNodePtr visit_statement(const AstNodePtr& node);
    AstNodePtr visit_assignment(const AstNodePtr& node);
    AstNodePtr visit_if(const AstNodePtr& node);
    AstNodePtr visit_while(const AstNodePtr& node);
    AstNodePtr visit_repeat(const AstNodePtr& node);
    AstNodePtr visit_for(const AstNodePtr& node);
    AstNodePtr visit_case(const AstNodePtr& node);
    AstNodePtr visit_call(const AstNodePtr& node, ValueInfo* out = nullptr);

    int resolve_type_node(const AstNodePtr& node);
    int resolve_array_type(const AstNodePtr& node);
    int resolve_range_type(const AstNodePtr& node);
    int resolve_enum_type(const AstNodePtr& node);
    int resolve_record_type(const AstNodePtr& node);

    ValueInfo eval_constant(const AstNodePtr& node);
    ValueInfo eval_expression(const AstNodePtr& node);
    ValueInfo eval_variable(const AstNodePtr& node, bool as_assignment_target = false);
    ValueInfo value_from_literal(const AstNodePtr& node);
};

}
