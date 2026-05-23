#include "semantic_analyzer.hpp"

#include <algorithm>

namespace semantic {

// Collect parameter types
std::vector<int> SemanticAnalyzer::parameter_types_for(const TabEntry& entry) const {
    std::vector<int> parameter_types;
    if (!result ||
        entry.ref <= 0 ||
        static_cast<size_t>(entry.ref) >= result->symbols.btab_entries().size()) {
        return parameter_types;
    }

    int parameter = result->symbols.btab_entries()[static_cast<size_t>(entry.ref)].lpar;
    while (parameter > 0 &&
           static_cast<size_t>(parameter) < result->symbols.tab_entries().size() &&
           result->symbols.tab_entry(parameter).obj == SymbolObject::Parameter) {
        parameter_types.push_back(result->symbols.tab_entry(parameter).type);
        parameter = result->symbols.tab_entry(parameter).link;
    }
    std::reverse(parameter_types.begin(), parameter_types.end());
    return parameter_types;
}

// Check assignment target
bool SemanticAnalyzer::is_assignable_storage(const ValueInfo& target,
                                             bool allow_current_function) const {
    if (!result ||
        target.base_tab_index < 0 ||
        static_cast<size_t>(target.base_tab_index) >= result->symbols.tab_entries().size()) {
        return false;
    }

    const auto& entry = result->symbols.tab_entry(target.base_tab_index);
    if (entry.obj == SymbolObject::Variable || entry.obj == SymbolObject::Parameter) {
        return true;
    }
    return allow_current_function &&
           entry.obj == SymbolObject::Function &&
           entry.ref == result->symbols.current_block_index();
}

// Check IO value type
bool SemanticAnalyzer::is_scalar_or_string_type(int type_id) const {
    if (!result || type_id == 0) return true;
    if (type_id < 0 || static_cast<size_t>(type_id) >= result->types.entries().size()) {
        return false;
    }
    const TypeKind kind = result->types.get(type_id).kind;
    return kind != TypeKind::None &&
           kind != TypeKind::Error &&
           kind != TypeKind::Array &&
           kind != TypeKind::Record;
}

// Mark assigned storage
void SemanticAnalyzer::mark_target_initialized(const ValueInfo& target) {
    if (!result) return;
    if (target.tab_index >= 0 &&
        static_cast<size_t>(target.tab_index) < result->symbols.tab_entries().size()) {
        result->symbols.tab_mutable(target.tab_index).initialized = true;
    }
    if (target.base_tab_index >= 0 &&
        static_cast<size_t>(target.base_tab_index) < result->symbols.tab_entries().size()) {
        result->symbols.tab_mutable(target.base_tab_index).initialized = true;
    }
}

// Validate user call
void SemanticAnalyzer::validate_user_call_arguments(
    const AstNodePtr& node,
    const TabEntry& entry,
    const std::vector<ValueInfo>& arguments) {
    const std::vector<int> parameter_types = parameter_types_for(entry);
    if (parameter_types.size() != arguments.size()) {
        semantic_error(node, "Call to '" + node->name + "' expects " +
                             std::to_string(parameter_types.size()) +
                             " argument(s), got " + std::to_string(arguments.size()));
    }

    const size_t checked = std::min(parameter_types.size(), arguments.size());
    for (size_t i = 0; i < checked; ++i) {
        if (!result->types.assignment_compatible(
                parameter_types[i], arguments[i].type,
                arguments[i].is_constant
                    ? std::optional<ConstantValue>(ConstantValue{
                          arguments[i].type, arguments[i].string_value,
                          arguments[i].int_value, arguments[i].real_value,
                          arguments[i].char_value, arguments[i].bool_value})
                    : std::nullopt)) {
            semantic_error(node->children[i],
                           "Argument " + std::to_string(i + 1) + " of '" +
                               node->name + "' expects " +
                               result->types.type_name(parameter_types[i]) +
                               ", got " + result->types.type_name(arguments[i].type));
        }
    }
}

// Validate built-in call
bool SemanticAnalyzer::validate_builtin_call(const AstNodePtr& node,
                                             const TabEntry& entry,
                                             ValueInfo* out) {
    const std::string name = SymbolTable::normalize(entry.identifier);
    if (name != "readln" && name != "writeln") {
        return false;
    }

    if (name == "readln") {
        for (size_t i = 0; i < node->children.size(); ++i) {
            const auto& child = node->children[i];
            if (!child || child->kind != AstKind::Variable) {
                semantic_error(child ? child : node,
                               "readln argument " + std::to_string(i + 1) +
                                   " must be an assignable variable");
                continue;
            }

            ValueInfo target = eval_variable(child, true);
            const bool assignable = is_assignable_storage(target, false);
            if (!assignable) {
                semantic_error(child, "readln argument " + std::to_string(i + 1) +
                                      " is not an assignable variable");
            }
            if (!is_scalar_or_string_type(target.type)) {
                semantic_error(child, "readln argument " + std::to_string(i + 1) +
                                      " must be a scalar or String target");
            }
            if (assignable && is_scalar_or_string_type(target.type)) {
                mark_target_initialized(target);
            }
            child->annotation.type_id = target.type;
        }
    } else {
        for (size_t i = 0; i < node->children.size(); ++i) {
            ValueInfo arg = eval_expression(node->children[i]);
            if (node->children[i]) node->children[i]->annotation.type_id = arg.type;
            if (!is_scalar_or_string_type(arg.type)) {
                semantic_error(node->children[i],
                               "writeln argument " + std::to_string(i + 1) +
                                   " must be a scalar or String expression");
            }
        }
    }

    if (out) {
        semantic_error(node, "Procedure '" + node->name + "' does not produce a value");
        out->type = entry.type;
        out->tab_index = node->annotation.tab_index;
        out->base_tab_index = node->annotation.tab_index;
        out->initialized = true;
    }

    return true;
}

// Visit assignment statement
AstNodePtr SemanticAnalyzer::visit_assignment(const AstNodePtr& node) {
    if (!node || node->children.size() < 2) return node;

    ValueInfo target = eval_variable(node->children[0], true);
    ValueInfo value = eval_expression(node->children[1]);

    node->annotation.type_id = target.type;
    node->annotation.tab_index = target.tab_index;

    bool assignable = false;
    std::string target_name = node->children[0] ? node->children[0]->name : "<target>";
    if (target.base_tab_index >= 0 &&
        static_cast<size_t>(target.base_tab_index) < result->symbols.tab_entries().size()) {
        const auto& entry = result->symbols.tab_entry(target.base_tab_index);
        assignable = is_assignable_storage(target, true);
        if (!assignable) {
            semantic_error(node, "Identifier '" + target_name + "' is not an assignable target (" +
                                 symbol_object_name(entry.obj) + ")");
        }
    }

    const bool compatible = result->types.assignment_compatible(
        target.type, value.type,
        value.is_constant
            ? std::optional<ConstantValue>(ConstantValue{value.type, value.string_value,
                                                         value.int_value, value.real_value,
                                                         value.char_value, value.bool_value})
            : std::nullopt);
    if (!compatible) {
        semantic_error(node, "Cannot assign " + result->types.type_name(value.type) +
                             " to " + result->types.type_name(target.type));
    }

    if (assignable && compatible && value.type != 0) {
        mark_target_initialized(target);
    }

    return node;
}

// Visit if statement
AstNodePtr SemanticAnalyzer::visit_if(const AstNodePtr& node) {
    if (!node || node->children.empty()) return node;
    ValueInfo cond = eval_expression(node->children[0]);
    if (cond.type != result->types.boolean_type()) {
        semantic_error(node, "if condition must be Boolean");
    }
    node->annotation.type_id = cond.type;
    for (size_t i = 1; i < node->children.size(); ++i) {
        visit_statement(node->children[i]);
    }
    return node;
}

// Visit while statement
AstNodePtr SemanticAnalyzer::visit_while(const AstNodePtr& node) {
    if (!node || node->children.empty()) return node;
    ValueInfo cond = eval_expression(node->children[0]);
    if (cond.type != result->types.boolean_type()) {
        semantic_error(node, "while condition must be Boolean");
    }
    node->annotation.type_id = cond.type;
    if (node->children.size() > 1) visit_compound_statement(node->children[1]);
    return node;
}

// Visit repeat statement
AstNodePtr SemanticAnalyzer::visit_repeat(const AstNodePtr& node) {
    if (!node) return node;
    if (!node->children.empty()) visit_compound_statement(node->children.front());
    if (node->children.size() > 1) {
        ValueInfo cond = eval_expression(node->children[1]);
        if (cond.type != result->types.boolean_type()) {
            semantic_error(node, "repeat-until condition must be Boolean");
        }
        node->annotation.type_id = cond.type;
    }
    return node;
}

// Visit for statement
AstNodePtr SemanticAnalyzer::visit_for(const AstNodePtr& node) {
    if (!node) return node;
    const int idx = result->symbols.lookup(node->name);
    if (idx < 0) {
        semantic_error(node, "for iterator '" + node->name + "' is not declared");
    } else {
        const auto& entry = result->symbols.tab_entry(idx);
        if (entry.obj != SymbolObject::Variable && entry.obj != SymbolObject::Parameter) {
            semantic_error(node, "for iterator '" + node->name + "' must be an assignable variable");
        } else if (!result->types.simple_non_real(entry.type)) {
            semantic_error(node, "for iterator '" + node->name + "' must be a simple ordinal non-Real type");
        }
    }

    for (size_t i = 0; i < node->children.size() && i < 2; ++i) {
        ValueInfo bound = eval_expression(node->children[i]);
        if (idx >= 0) {
            const auto& entry = result->symbols.tab_entry(idx);
            if (!result->types.assignment_compatible(
                    entry.type, bound.type,
                    bound.is_constant
                        ? std::optional<ConstantValue>(ConstantValue{bound.type, bound.string_value,
                                                                     bound.int_value, bound.real_value,
                                                                     bound.char_value, bound.bool_value})
                        : std::nullopt)) {
                semantic_error(node->children[i],
                               "for bound type is incompatible with iterator '" + node->name + "'");
            }
        }
    }

    node->annotation.tab_index = idx;
    if (idx >= 0 && static_cast<size_t>(idx) < result->symbols.tab_entries().size()) {
        result->symbols.tab_mutable(idx).initialized = true;
    }
    if (node->children.size() > 2) visit_compound_statement(node->children[2]);
    return node;
}

// Visit case statement
AstNodePtr SemanticAnalyzer::visit_case(const AstNodePtr& node) {
    if (!node || node->children.empty()) return node;
    ValueInfo selector = eval_expression(node->children.front());
    node->annotation.type_id = selector.type;

    for (size_t i = 1; i < node->children.size(); ++i) {
        const auto& branch = node->children[i];
        if (!branch || branch->kind != AstKind::CaseBranch) continue;
        for (const auto& child : branch->children) {
            if (!child) continue;
            if (child->kind == AstKind::Literal || child->kind == AstKind::UnaryOp) {
                ValueInfo label = eval_constant(child);
                if (!result->types.compatible(selector.type, label.type)) {
                    semantic_error(child, "case label type " + result->types.type_name(label.type) +
                                           " is incompatible with selector " +
                                           result->types.type_name(selector.type));
                }
            } else {
                visit_statement(child);
            }
        }
    }
    return node;
}

// Visit callable node
AstNodePtr SemanticAnalyzer::visit_call(const AstNodePtr& node, ValueInfo* out) {
    if (!node) return node;
    const int idx = result->symbols.lookup(node->name);
    std::vector<ValueInfo> arguments;

    if (idx < 0) {
        semantic_error(node, "Procedure/function '" + node->name + "' is not declared");
    } else {
        const auto& entry = result->symbols.tab_entry(idx);
        if (entry.obj != SymbolObject::Procedure && entry.obj != SymbolObject::Function) {
            semantic_error(node, "Identifier '" + node->name + "' is not callable");
        }
        node->annotation.tab_index = idx;
        node->annotation.type_id = entry.type;
    }

    if (idx >= 0 && static_cast<size_t>(idx) < result->symbols.tab_entries().size()) {
        const auto& entry = result->symbols.tab_entry(idx);
        if (validate_builtin_call(node, entry, out)) {
            return node;
        }
    }

    for (const auto& child : node->children) {
        ValueInfo arg = eval_expression(child);
        arguments.push_back(arg);
        if (child) child->annotation.type_id = arg.type;
    }

    if (idx >= 0 && static_cast<size_t>(idx) < result->symbols.tab_entries().size()) {
        const auto& entry = result->symbols.tab_entry(idx);
        if (entry.obj == SymbolObject::Procedure || entry.obj == SymbolObject::Function) {
            validate_user_call_arguments(node, entry, arguments);
        }

        if (out && entry.obj == SymbolObject::Procedure) {
            semantic_error(node, "Procedure '" + node->name + "' does not produce a value");
        }
    }

    if (out) {
        out->type = node->annotation.type_id;
        out->tab_index = idx;
        out->base_tab_index = idx;
        out->initialized = true;
    }
    return node;
}

}
