#pragma once

#include <sstream>
#include <string>
#include <utility>

namespace backend {

enum class RuntimeValueKind {
    Empty,
    Integer,
    Real,
    Char,
    Boolean,
    String
};

struct RuntimeValue {
    RuntimeValueKind kind = RuntimeValueKind::Empty;
    int int_value = 0;
    double real_value = 0.0;
    char char_value = '\0';
    bool bool_value = false;
    std::string string_value;

    static RuntimeValue empty() {
        return RuntimeValue{};
    }

    static RuntimeValue integer(int value) {
        RuntimeValue result;
        result.kind = RuntimeValueKind::Integer;
        result.int_value = value;
        return result;
    }

    static RuntimeValue real(double value) {
        RuntimeValue result;
        result.kind = RuntimeValueKind::Real;
        result.real_value = value;
        return result;
    }

    static RuntimeValue character(char value) {
        RuntimeValue result;
        result.kind = RuntimeValueKind::Char;
        result.char_value = value;
        return result;
    }

    static RuntimeValue boolean(bool value) {
        RuntimeValue result;
        result.kind = RuntimeValueKind::Boolean;
        result.bool_value = value;
        return result;
    }

    static RuntimeValue string(std::string value) {
        RuntimeValue result;
        result.kind = RuntimeValueKind::String;
        result.string_value = std::move(value);
        return result;
    }

    static std::string escape_code_text(const std::string& value) {
        std::string escaped;
        for (char ch : value) {
            switch (ch) {
                case '\\': escaped += "\\\\"; break;
                case '"': escaped += "\\\""; break;
                case '\'': escaped += "\\'"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += ch; break;
            }
        }
        return escaped;
    }

    std::string to_string() const {
        switch (kind) {
            case RuntimeValueKind::Integer:
                return std::to_string(int_value);
            case RuntimeValueKind::Real: {
                std::ostringstream out;
                out << real_value;
                return out.str();
            }
            case RuntimeValueKind::Char:
                return std::string(1, char_value);
            case RuntimeValueKind::Boolean:
                return bool_value ? "true" : "false";
            case RuntimeValueKind::String:
                return string_value;
            case RuntimeValueKind::Empty:
                return "<empty>";
        }
        return "<unknown>";
    }

    std::string to_code_literal() const {
        switch (kind) {
            case RuntimeValueKind::Integer:
            case RuntimeValueKind::Real:
            case RuntimeValueKind::Boolean:
                return to_string();
            case RuntimeValueKind::Char:
                return "'" + escape_code_text(std::string(1, char_value)) + "'";
            case RuntimeValueKind::String:
                return "\"" + escape_code_text(string_value) + "\"";
            case RuntimeValueKind::Empty:
                return "<empty>";
        }
        return "<unknown>";
    }
};

}
