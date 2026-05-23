#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace semantic {

// Semantic type categories
enum class TypeKind {
    None,
    Error,
    Integer,
    Real,
    Char,
    Boolean,
    String,
    Subrange,
    Enumerated,
    Array,
    Record
};

// Compile-time constant value
struct ConstantValue {
    int type_id = 0;
    std::string text;
    int int_value = 0;
    double real_value = 0.0;
    char char_value = '\0';
    bool bool_value = false;
};

// Type metadata entry
struct TypeInfo {
    TypeKind kind = TypeKind::None;
    std::string name;
    int base_type = 0;
    int ref = 0;
    int low = 0;
    int high = 0;
    int length = 0;
    bool anonymous = false;
    std::unordered_map<std::string, int> fields;
};

// Semantic type registry
class TypeRegistry {
public:
    TypeRegistry();

    int none_type() const;
    int integer_type() const;
    int real_type() const;
    int char_type() const;
    int boolean_type() const;
    int string_type() const;

    int add_type(const TypeInfo& type);
    const TypeInfo& get(int type_id) const;
    TypeInfo& get_mutable(int type_id);
    const std::vector<TypeInfo>& entries() const;

    std::string type_name(int type_id) const;
    int type_size(int type_id) const;

    bool compatible(int left_type, int right_type) const;
    bool assignment_compatible(int target_type, int value_type,
                               const std::optional<ConstantValue>& value = std::nullopt) const;
    bool numeric(int type_id) const;
    bool simple_non_real(int type_id) const;

private:
    std::vector<TypeInfo> types;
    int integer_id = 0;
    int real_id = 0;
    int char_id = 0;
    int boolean_id = 0;
    int string_id = 0;
};

std::string type_kind_name(TypeKind kind);

}
