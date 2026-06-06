#include "semantic_analyzer.hpp"

#include <algorithm>

namespace semantic {

// Visit constant declaration
AstNodePtr SemanticAnalyzer::visit_const_decl(const AstNodePtr& node) {
    ValueInfo value = node && !node->children.empty() ? eval_constant(node->children.front()) : ValueInfo{};
    const int idx = add_symbol_checked(node, node->name, SymbolObject::Constant,
                                       value.type, 0, 1, true, value.string_value);
    node->annotation.type_id = value.type;
    node->annotation.tab_index = idx;
    node->annotation.lexical_level = result->symbols.current_level();
    return node;
}

// Visit type declaration
AstNodePtr SemanticAnalyzer::visit_type_decl(const AstNodePtr& node) {
    int resolved = node && !node->children.empty() ? resolve_type_node(node->children.front()) : 0;
    if (resolved > 0 && static_cast<size_t>(resolved) < result->types.entries().size()) {
        auto& info = result->types.get_mutable(resolved);
        if (info.anonymous) {
            info.name = node->name;
            info.anonymous = false;
        }
    }
    const int idx = add_symbol_checked(node, node->name, SymbolObject::Type,
                                       resolved, type_ref(resolved), 1, true);
    node->annotation.type_id = resolved;
    node->annotation.tab_index = idx;
    node->annotation.lexical_level = result->symbols.current_level();
    return node;
}

// Visit variable declaration
AstNodePtr SemanticAnalyzer::visit_var_decl(const AstNodePtr& node) {
    int resolved = node && !node->children.empty() ? resolve_type_node(node->children.front()) : 0;
    const int idx = add_symbol_checked(node, node->name, SymbolObject::Variable,
                                       resolved, type_ref(resolved), 1, false);
    node->annotation.type_id = resolved;
    node->annotation.tab_index = idx;
    node->annotation.lexical_level = result->symbols.current_level();
    return node;
}

// Visit subprogram declaration
AstNodePtr SemanticAnalyzer::visit_subprogram_decl(const AstNodePtr& node) {
    if (!node) return nullptr;
    const bool is_function = node->kind == AstKind::FunctionDecl;
    int result_type = 0;

    if (is_function && !node->children.empty() && node->children.front()->kind == AstKind::TypeRef) {
        result_type = resolve_type_node(node->children.front());
    }

    const int block_index = result->symbols.add_block();
    const int sub_idx = add_symbol_checked(node, node->name,
                                           is_function ? SymbolObject::Function : SymbolObject::Procedure,
                                           result_type, block_index, 1, true);
    node->annotation.type_id = result_type;
    node->annotation.tab_index = sub_idx;
    node->annotation.block_index = block_index;
    node->annotation.lexical_level = result->symbols.current_level();

    result->symbols.push_scope(block_index);

    for (const auto& child : node->children) {
        if (!child) continue;
        if (child->kind == AstKind::ParameterGroup) {
            for (const auto& param : child->children) {
                if (!param || param->kind != AstKind::VarDecl) continue;
                int param_type = param->children.empty() ? 0 : resolve_type_node(param->children.front());
                const int global_idx = result->symbols.lookup_global_const_or_var(param->name);
                if (global_idx >= 0) {
                    const auto& entry = result->symbols.tab_entry(global_idx);
                    semantic_error(param, "Formal parameter '" + param->name +
                                   "' cannot reuse global " + symbol_object_name(entry.obj) + " name");
                    continue;
                }
                const int idx = add_symbol_checked(param, param->name, SymbolObject::Parameter,
                                                   param_type, type_ref(param_type), 1, true);
                result->symbols.btab_mutable(block_index).lpar = idx;
                result->symbols.btab_mutable(block_index).psze += type_size(param_type);
                param->annotation.type_id = param_type;
                param->annotation.tab_index = idx;
                param->annotation.lexical_level = result->symbols.current_level();
            }
        } else if (child->kind == AstKind::DeclarationPart) {
            visit_declaration_part(child);
        } else if (child->kind == AstKind::CompoundStatement) {
            visit_compound_statement(child);
        }
    }

    result->symbols.pop_scope();
    return node;
}

// Resolve type node
int SemanticAnalyzer::resolve_type_node(const AstNodePtr& node) {
    if (!node) return 0;
    switch (node->kind) {
        case AstKind::TypeRef: {
            const int idx = result->symbols.lookup(node->name);
            if (idx < 0 || result->symbols.tab_entry(idx).obj != SymbolObject::Type) {
                semantic_error(node, "Type '" + node->name + "' is not declared");
                return 0;
            }
            node->annotation.type_id = result->symbols.tab_entry(idx).type;
            node->annotation.tab_index = idx;
            return result->symbols.tab_entry(idx).type;
        }
        case AstKind::ArrayType:
            return resolve_array_type(node);
        case AstKind::RangeType:
            return resolve_range_type(node);
        case AstKind::EnumType:
            return resolve_enum_type(node);
        case AstKind::RecordType:
            return resolve_record_type(node);
        default:
            return 0;
    }
}

// Resolve array type
int SemanticAnalyzer::resolve_array_type(const AstNodePtr& node) {
    int index_type = 0;
    int low = 0;
    int high = 0;
    int element_type = 0;

    if (node && !node->children.empty()) {
        index_type = resolve_type_node(node->children[0]);
        if (index_type > 0 && static_cast<size_t>(index_type) < result->types.entries().size()) {
            const auto& info = result->types.get(index_type);
            if (info.kind == TypeKind::Subrange) {
                low = info.low;
                high = info.high;
            }
        }
    }
    if (node && node->children.size() > 1) {
        element_type = resolve_type_node(node->children[1]);
    }

    if (!result->types.simple_non_real(index_type)) {
        semantic_error(node, "Array index type must be a simple non-Real type");
    }

    const int elem_size = type_size(element_type);
    const int array_ref = result->symbols.add_array_entry(index_type, element_type, low,
                                                          high, elem_size,
                                                          type_ref(element_type));

    TypeInfo type;
    type.kind = TypeKind::Array;
    type.name = "anonymous_array";
    type.base_type = element_type;
    type.ref = array_ref;
    type.anonymous = true;
    const int type_id = result->types.add_type(type);
    node->annotation.type_id = type_id;
    return type_id;
}

// Resolve range type
int SemanticAnalyzer::resolve_range_type(const AstNodePtr& node) {
    std::vector<ValueInfo> constants;
    if (node) {
        for (const auto& child : node->children) {
            constants.push_back(eval_constant(child));
        }
    }
    if (constants.size() != 2) return 0;

    if (constants[0].type != constants[1].type) {
        semantic_error(node, "Range bounds must have the same type");
    }
    if (constants[0].type == result->types.real_type()) {
        semantic_error(node, "Subrange cannot use Real bounds");
    }

    int low = 0;
    int high = 0;
    if (constants[0].type == result->types.integer_type()) {
        low = constants[0].int_value;
        high = constants[1].int_value;
    } else if (constants[0].type == result->types.char_type()) {
        low = constants[0].char_value;
        high = constants[1].char_value;
    }
    if (low > high) semantic_error(node, "Range lower bound is greater than upper bound");

    TypeInfo type;
    type.kind = TypeKind::Subrange;
    type.name = "subrange(" + result->types.type_name(constants[0].type) + ")";
    type.base_type = constants[0].type;
    type.low = low;
    type.high = high;
    type.anonymous = true;
    const int type_id = result->types.add_type(type);
    node->annotation.type_id = type_id;
    return type_id;
}

// Resolve enum type
int SemanticAnalyzer::resolve_enum_type(const AstNodePtr& node) {
    int common_type = 0;
    if (node) {
        for (const auto& child : node->children) {
            if (!child || child->kind != AstKind::Literal) continue;
            const std::string id = child->value;
            const int idx = result->symbols.lookup(id);
            if (idx < 0) {
                semantic_error(child, "Enumerated identifier '" + id +
                                      "' must be declared before the enumerated type");
                continue;
            }
            const auto& entry = result->symbols.tab_entry(idx);
            if (entry.obj != SymbolObject::Constant) {
                semantic_error(child, "Enumerated identifier '" + id + "' must be a constant");
                continue;
            }
            const int current_type = entry.type;
            if (common_type == 0) common_type = current_type;
            else if (!result->types.compatible(common_type, current_type)) {
                semantic_error(child, "Enumerated identifiers must have compatible types");
            }
        }
    }

    TypeInfo type;
    type.kind = TypeKind::Enumerated;
    type.name = "enumerated";
    type.base_type = common_type;
    type.anonymous = true;
    const int type_id = result->types.add_type(type);
    node->annotation.type_id = type_id;
    return type_id;
}

// Resolve record type
int SemanticAnalyzer::resolve_record_type(const AstNodePtr& node) {
    const int block_index = result->symbols.add_block();
    result->symbols.push_scope(block_index);

    TypeInfo type;
    type.kind = TypeKind::Record;
    type.name = "anonymous_record";
    type.ref = block_index;
    type.anonymous = true;

    if (node) {
        for (const auto& field : node->children) {
            if (!field || field->kind != AstKind::FieldDecl) continue;
            int field_type = field->children.empty() ? 0 : resolve_type_node(field->children.front());
            const int existing = result->symbols.lookup_current_scope(field->name);
            if (existing != -1) {
                semantic_error(field, "Identifier '" + field->name + "' is already declared in the current scope");
                type.fields[field->name] = existing;
                continue;
            }
            const int idx = result->symbols.add_symbol(field->name, SymbolObject::Field,
                                                       field_type, type_ref(field_type),
                                                       1, true, "", type_size(field_type));
            field->annotation.type_id = field_type;
            field->annotation.tab_index = idx;
            type.fields[field->name] = idx;
        }
    }

    result->symbols.pop_scope();
    const int type_id = result->types.add_type(type);
    node->annotation.type_id = type_id;
    return type_id;
}

} 
