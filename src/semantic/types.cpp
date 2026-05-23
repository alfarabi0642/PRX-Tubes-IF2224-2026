#include "types.hpp"

#include <stdexcept>

namespace semantic {

namespace {
TypeInfo make_builtin(TypeKind kind, const std::string& name) {
    TypeInfo type;
    type.kind = kind;
    type.name = name;
    return type;
}
}

TypeRegistry::TypeRegistry() {
    types.push_back(make_builtin(TypeKind::None, "none"));
    integer_id = add_type(make_builtin(TypeKind::Integer, "Integer"));
    real_id = add_type(make_builtin(TypeKind::Real, "Real"));
    char_id = add_type(make_builtin(TypeKind::Char, "Char"));
    boolean_id = add_type(make_builtin(TypeKind::Boolean, "Boolean"));
    string_id = add_type(make_builtin(TypeKind::String, "String"));
}

int TypeRegistry::none_type() const { return 0; }
int TypeRegistry::integer_type() const { return integer_id; }
int TypeRegistry::real_type() const { return real_id; }
int TypeRegistry::char_type() const { return char_id; }
int TypeRegistry::boolean_type() const { return boolean_id; }
int TypeRegistry::string_type() const { return string_id; }

int TypeRegistry::add_type(const TypeInfo& type) {
    types.push_back(type);
    return static_cast<int>(types.size()) - 1;
}

const TypeInfo& TypeRegistry::get(int type_id) const {
    if (type_id < 0 || static_cast<size_t>(type_id) >= types.size()) {
        throw std::out_of_range("invalid semantic type id");
    }
    return types[static_cast<size_t>(type_id)];
}

TypeInfo& TypeRegistry::get_mutable(int type_id) {
    if (type_id < 0 || static_cast<size_t>(type_id) >= types.size()) {
        throw std::out_of_range("invalid semantic type id");
    }
    return types[static_cast<size_t>(type_id)];
}

const std::vector<TypeInfo>& TypeRegistry::entries() const {
    return types;
}

std::string TypeRegistry::type_name(int type_id) const {
    if (type_id > 0 && static_cast<size_t>(type_id) < types.size()) {
        return types[static_cast<size_t>(type_id)].name;
    }
    return "unknown";
}

int TypeRegistry::type_size(int type_id) const {
    if (type_id <= 0 || static_cast<size_t>(type_id) >= types.size()) return 1;
    return types[static_cast<size_t>(type_id)].kind == TypeKind::Real ? 2 : 1;
}

bool TypeRegistry::compatible(int left_type, int right_type) const {
    if (left_type == 0 || right_type == 0) return true;
    if (left_type == right_type) return true;

    auto base = [&](int type_id) {
        if (type_id > 0 && static_cast<size_t>(type_id) < types.size()) {
            const auto& type = types[static_cast<size_t>(type_id)];
            if (type.kind == TypeKind::Subrange || type.kind == TypeKind::Enumerated) {
                return type.base_type;
            }
        }
        return type_id;
    };

    return base(left_type) == base(right_type);
}

bool TypeRegistry::assignment_compatible(int target_type, int value_type,
                                         const std::optional<ConstantValue>& value) const {
    if (target_type == 0 || value_type == 0) return true;
    if (target_type == real_id && value_type == integer_id) return true;
    if (!compatible(target_type, value_type)) return false;

    if (target_type > 0 && static_cast<size_t>(target_type) < types.size() &&
        types[static_cast<size_t>(target_type)].kind == TypeKind::Subrange &&
        value.has_value()) {
        const auto& type = types[static_cast<size_t>(target_type)];
        int scalar = 0;
        if (value->type_id == integer_id) scalar = value->int_value;
        else if (value->type_id == char_id) scalar = value->char_value;
        return scalar >= type.low && scalar <= type.high;
    }

    return true;
}

bool TypeRegistry::numeric(int type_id) const {
    return type_id == integer_id || type_id == real_id ||
           (type_id > 0 && static_cast<size_t>(type_id) < types.size() &&
            types[static_cast<size_t>(type_id)].kind == TypeKind::Subrange &&
            types[static_cast<size_t>(type_id)].base_type == integer_id);
}

bool TypeRegistry::simple_non_real(int type_id) const {
    if (type_id == integer_id || type_id == char_id || type_id == boolean_id) return true;
    if (type_id > 0 && static_cast<size_t>(type_id) < types.size()) {
        const auto& type = types[static_cast<size_t>(type_id)];
        return (type.kind == TypeKind::Subrange && type.base_type != real_id) ||
               type.kind == TypeKind::Enumerated;
    }
    return false;
}

std::string type_kind_name(TypeKind kind) {
    switch (kind) {
        case TypeKind::None: return "None";
        case TypeKind::Error: return "Error";
        case TypeKind::Integer: return "Integer";
        case TypeKind::Real: return "Real";
        case TypeKind::Char: return "Char";
        case TypeKind::Boolean: return "Boolean";
        case TypeKind::String: return "String";
        case TypeKind::Subrange: return "Subrange";
        case TypeKind::Enumerated: return "Enumerated";
        case TypeKind::Array: return "Array";
        case TypeKind::Record: return "Record";
    }
    return "Unknown";
}

} // namespace semantic
