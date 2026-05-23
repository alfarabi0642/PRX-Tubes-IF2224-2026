#include "semantic_analyzer.hpp"

#include <cstdlib>
#include <exception>

namespace semantic {

ValueInfo SemanticAnalyzer::eval_constant(const AstNodePtr& node) {
    if (!node) return {};
    if (node->kind == AstKind::UnaryOp && !node->children.empty()) {
        ValueInfo value = eval_constant(node->children.front());
        if (node->op == "minus") {
            if (value.type == result->types.integer_type()) value.int_value *= -1;
            else if (value.type == result->types.real_type()) value.real_value *= -1.0;
            else if (value.type != 0) semantic_error(node, "Unary minus requires a numeric constant");
            if (!value.string_value.empty() && value.string_value[0] != '-') {
                value.string_value = "-" + value.string_value;
            }
        }
        return value;
    }
    if (node->kind == AstKind::Literal) return value_from_literal(node);
    return eval_expression(node);
}

ValueInfo SemanticAnalyzer::eval_expression(const AstNodePtr& node) {
    if (!node) return {};

    if (node->kind == AstKind::Literal) return value_from_literal(node);
    if (node->kind == AstKind::Variable) return eval_variable(node);
    if (node->kind == AstKind::Call) {
        ValueInfo out;
        visit_call(node, &out);
        return out;
    }
    if (node->kind == AstKind::UnaryOp) {
        ValueInfo value = node->children.empty() ? ValueInfo{} : eval_expression(node->children.front());
        if (node->op == "notsy") {
            if (value.type != 0 && value.type != result->types.boolean_type()) {
                semantic_error(node, "Operator not requires a Boolean operand");
            }
            value.type = result->types.boolean_type();
            value.is_constant = false;
        } else if (node->op == "minus") {
            if (!result->types.numeric(value.type) && value.type != 0) {
                semantic_error(node, "Unary minus requires a numeric operand");
            }
            if (value.type == result->types.integer_type()) value.int_value *= -1;
            if (value.type == result->types.real_type()) value.real_value *= -1.0;
        }
        node->annotation.type_id = value.type;
        node->annotation.tab_index = value.tab_index;
        return value;
    }
    if (node->kind != AstKind::BinaryOp || node->children.size() < 2) return {};

    ValueInfo left = eval_expression(node->children[0]);
    ValueInfo right = eval_expression(node->children[1]);
    ValueInfo result_value;
    result_value.type = left.type;
    result_value.is_constant = false;

    const std::string& op = node->op;
    const bool relational = op == "eql" || op == "neq" || op == "gtr" ||
                            op == "geq" || op == "lss" || op == "leq";
    if (relational) {
        auto orderable = [&](int type_id) {
            if (type_id == result->types.integer_type() ||
                type_id == result->types.real_type() ||
                type_id == result->types.char_type() ||
                type_id == result->types.boolean_type()) {
                return true;
            }
            if (type_id > 0 && static_cast<size_t>(type_id) < result->types.entries().size()) {
                const auto& info = result->types.get(type_id);
                return info.kind == TypeKind::Subrange || info.kind == TypeKind::Enumerated;
            }
            return false;
        };

        if (!result->types.compatible(left.type, right.type)) {
            semantic_error(node, "Relational operands are incompatible: " +
                                 result->types.type_name(left.type) + " and " +
                                 result->types.type_name(right.type));
        } else if (op != "eql" && op != "neq" && (!orderable(left.type) || !orderable(right.type))) {
            semantic_error(node, "Relational operator " + op + " requires orderable operands");
        }
        result_value.type = result->types.boolean_type();
    } else if (op == "orsy") {
        if (left.type != result->types.boolean_type() || right.type != result->types.boolean_type()) {
            semantic_error(node, "Operator or requires Boolean operands");
        }
        result_value.type = result->types.boolean_type();
    } else if (op == "andsy") {
        if (left.type != result->types.boolean_type() || right.type != result->types.boolean_type()) {
            semantic_error(node, "Operator and requires Boolean operands");
        }
        result_value.type = result->types.boolean_type();
    } else if (op == "idiv" || op == "imod") {
        auto integer_like = [&](int type_id) {
            if (type_id == result->types.integer_type()) return true;
            return type_id > 0 && static_cast<size_t>(type_id) < result->types.entries().size() &&
                   result->types.get(type_id).kind == TypeKind::Subrange &&
                   result->types.get(type_id).base_type == result->types.integer_type();
        };
        if (!integer_like(left.type) || !integer_like(right.type)) {
            semantic_error(node, "Operator " + op + " requires Integer operands");
        }
        result_value.type = result->types.integer_type();
    } else {
        if (!result->types.numeric(left.type) || !result->types.numeric(right.type)) {
            semantic_error(node, "Operator " + op + " requires numeric operands");
            result_value.type = 0;
        } else {
            result_value.type = (op == "rdiv" ||
                                 left.type == result->types.real_type() ||
                                 right.type == result->types.real_type())
                                    ? result->types.real_type()
                                    : result->types.integer_type();
        }
    }

    node->annotation.type_id = result_value.type;
    return result_value;
}

ValueInfo SemanticAnalyzer::eval_variable(const AstNodePtr& node, bool as_assignment_target) {
    ValueInfo value;
    if (!node) return value;

    const int idx = result->symbols.lookup(node->name);
    if (idx < 0) {
        semantic_error(node, "Identifier '" + node->name + "' is not declared");
        return value;
    }

    const auto& base_entry = result->symbols.tab_entry(idx);
    value.type = base_entry.type;
    value.tab_index = idx;
    value.base_tab_index = idx;
    value.initialized = base_entry.initialized;

    if (!as_assignment_target && base_entry.obj == SymbolObject::Function) {
        validate_user_call_arguments(node, base_entry, {});
        value.initialized = true;
        if (node->children.empty()) {
            node->annotation.type_id = value.type;
            node->annotation.tab_index = value.tab_index;
            node->annotation.lexical_level = result->symbols.current_level();
            node->annotation.initialized = true;
            return value;
        }
    }

    if (!as_assignment_target &&
        base_entry.obj != SymbolObject::Variable &&
        base_entry.obj != SymbolObject::Parameter &&
        base_entry.obj != SymbolObject::Constant &&
        base_entry.obj != SymbolObject::Function) {
        semantic_error(node, "Identifier '" + node->name + "' is not a value (" +
                             symbol_object_name(base_entry.obj) + ")");
        value.type = 0;
        return value;
    }

    if (base_entry.obj == SymbolObject::Constant) {
        value.is_constant = true;
        value.string_value = base_entry.value;
        const std::string lowered_name = SymbolTable::normalize(node->name);
        const std::string lowered_text = SymbolTable::normalize(base_entry.value);
        try {
            if (value.type == result->types.integer_type() && !base_entry.value.empty()) {
                value.int_value = std::stoi(base_entry.value);
            } else if (value.type == result->types.real_type() && !base_entry.value.empty()) {
                value.real_value = std::stod(base_entry.value);
            } else if (value.type == result->types.char_type() && base_entry.value.size() >= 3) {
                value.char_value = base_entry.value[1];
            } else if (value.type == result->types.boolean_type()) {
                value.bool_value = lowered_name == "true" || lowered_text == "true";
            }
        } catch (const std::exception&) {
            semantic_error(node, "Invalid constant value for '" + node->name + "'");
        }
    }

    if (!as_assignment_target && !value.initialized &&
        base_entry.obj == SymbolObject::Variable &&
        base_entry.lev == result->symbols.current_level()) {
        semantic_error(node, "Variable '" + node->name + "' may be used before initialization");
    }

    for (const auto& component : node->children) {
        if (!component) continue;
        if (component->kind == AstKind::IndexComponent) {
            for (const auto& index_node : component->children) {
                if (value.type <= 0 ||
                    static_cast<size_t>(value.type) >= result->types.entries().size() ||
                    result->types.get(value.type).kind != TypeKind::Array) {
                    semantic_error(component, "Indexed access requires an Array type");
                    value.type = 0;
                    break;
                }

                const int array_ref = result->types.get(value.type).ref;
                if (array_ref < 0 ||
                    static_cast<size_t>(array_ref) >= result->symbols.atab_entries().size()) {
                    semantic_error(component, "Array type is missing atab metadata");
                    value.type = 0;
                    break;
                }

                ValueInfo index = eval_expression(index_node);
                const auto& array = result->symbols.atab_entry(array_ref);
                if (!result->types.compatible(array.xtyp, index.type)) {
                    semantic_error(index_node, "Array index type " +
                                               result->types.type_name(index.type) +
                                               " is incompatible with " +
                                               result->types.type_name(array.xtyp));
                } else if (!result->types.assignment_compatible(
                               array.xtyp, index.type,
                               index.is_constant
                                   ? std::optional<ConstantValue>(ConstantValue{
                                         index.type, index.string_value, index.int_value,
                                         index.real_value, index.char_value, index.bool_value})
                                   : std::nullopt)) {
                    semantic_error(index_node, "Array index value is outside bounds " +
                                               std::to_string(array.low) + ".." +
                                               std::to_string(array.high));
                }
                value.type = array.etyp;
            }
        } else if (component->kind == AstKind::FieldComponent) {
            if (value.type <= 0 ||
                static_cast<size_t>(value.type) >= result->types.entries().size() ||
                result->types.get(value.type).kind != TypeKind::Record) {
                semantic_error(component, "Field access requires a Record type");
                value.type = 0;
                continue;
            }
            const auto& fields = result->types.get(value.type).fields;
            const auto found = fields.find(component->name);
            if (found == fields.end()) {
                semantic_error(component, "Record field '" + component->name + "' is not declared");
                value.type = 0;
            } else {
                value.tab_index = found->second;
                value.type = result->symbols.tab_entry(found->second).type;
            }
        }
    }

    node->annotation.type_id = value.type;
    node->annotation.tab_index = value.tab_index;
    node->annotation.lexical_level = result->symbols.current_level();
    node->annotation.initialized = value.initialized;
    return value;
}

ValueInfo SemanticAnalyzer::value_from_literal(const AstNodePtr& node) {
    ValueInfo value;
    if (!node) return value;
    value.is_constant = true;
    value.initialized = true;
    value.string_value = node->value;

    try {
        if (node->literal_kind == LiteralKind::Integer) {
            value.type = result->types.integer_type();
            value.int_value = std::stoi(node->value);
        } else if (node->literal_kind == LiteralKind::Real) {
            value.type = result->types.real_type();
            value.real_value = std::stod(node->value);
        } else if (node->literal_kind == LiteralKind::Char) {
            value.type = result->types.char_type();
            value.char_value = node->value.size() >= 3 ? node->value[1] : '\0';
        } else if (node->literal_kind == LiteralKind::String) {
            value.type = result->types.string_type();
            value.length = static_cast<int>(node->value.size());
        } else if (node->literal_kind == LiteralKind::IdentifierConstant) {
            const int idx = result->symbols.lookup(node->value);
            if (idx < 0) {
                semantic_error(node, "Identifier '" + node->value + "' is not declared");
            } else {
                const auto& entry = result->symbols.tab_entry(idx);
                if (entry.obj != SymbolObject::Variable &&
                    entry.obj != SymbolObject::Parameter &&
                    entry.obj != SymbolObject::Constant &&
                    entry.obj != SymbolObject::Function) {
                    semantic_error(node, "Identifier '" + node->value + "' is not a value (" +
                                         symbol_object_name(entry.obj) + ")");
                    return value;
                }
                value.type = entry.type;
                value.tab_index = idx;
                value.base_tab_index = idx;
                value.initialized = entry.initialized;
                value.is_constant = entry.obj == SymbolObject::Constant;
                value.string_value = entry.value;
                if (entry.obj == SymbolObject::Variable && !entry.initialized &&
                    entry.lev == result->symbols.current_level()) {
                    semantic_error(node, "Variable '" + node->value + "' may be used before initialization");
                }
                const std::string lowered_name = SymbolTable::normalize(node->value);
                const std::string lowered_text = SymbolTable::normalize(value.string_value);
                if (value.type == result->types.integer_type() && !value.string_value.empty()) {
                    value.int_value = std::stoi(value.string_value);
                } else if (value.type == result->types.real_type() && !value.string_value.empty()) {
                    value.real_value = std::stod(value.string_value);
                } else if (value.type == result->types.char_type() && value.string_value.size() >= 3) {
                    value.char_value = value.string_value[1];
                } else if (value.type == result->types.boolean_type()) {
                    value.bool_value = lowered_name == "true" || lowered_text == "true";
                }
            }
        }
    } catch (const std::exception&) {
        semantic_error(node, "Invalid literal value '" + node->value + "'");
    }

    node->annotation.type_id = value.type;
    node->annotation.tab_index = value.tab_index;
    node->annotation.initialized = value.initialized;
    return value;
}

} // namespace semantic
