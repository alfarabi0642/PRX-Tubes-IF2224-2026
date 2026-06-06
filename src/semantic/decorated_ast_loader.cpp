#include "decorated_ast_loader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace semantic {

namespace {

std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() &&
           std::isspace(static_cast<unsigned char>(text[first]))) {
        ++first;
    }

    std::size_t last = text.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(text[last - 1]))) {
        --last;
    }

    return text.substr(first, last - first);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() &&
           text.compare(0, prefix.size(), prefix) == 0;
}

bool parse_ast_kind(const std::string& text, AstKind* out) {
    static const std::unordered_map<std::string, AstKind> kinds = {
        {"Program", AstKind::Program},
        {"DeclarationPart", AstKind::DeclarationPart},
        {"ConstDecl", AstKind::ConstDecl},
        {"TypeDecl", AstKind::TypeDecl},
        {"VarDecl", AstKind::VarDecl},
        {"ProcedureDecl", AstKind::ProcedureDecl},
        {"FunctionDecl", AstKind::FunctionDecl},
        {"ParameterGroup", AstKind::ParameterGroup},
        {"CompoundStatement", AstKind::CompoundStatement},
        {"EmptyStatement", AstKind::EmptyStatement},
        {"AssignStatement", AstKind::AssignStatement},
        {"IfStatement", AstKind::IfStatement},
        {"CaseStatement", AstKind::CaseStatement},
        {"CaseBranch", AstKind::CaseBranch},
        {"WhileStatement", AstKind::WhileStatement},
        {"RepeatStatement", AstKind::RepeatStatement},
        {"ForStatement", AstKind::ForStatement},
        {"Call", AstKind::Call},
        {"ParameterList", AstKind::ParameterList},
        {"Variable", AstKind::Variable},
        {"IndexComponent", AstKind::IndexComponent},
        {"FieldComponent", AstKind::FieldComponent},
        {"Literal", AstKind::Literal},
        {"UnaryOp", AstKind::UnaryOp},
        {"BinaryOp", AstKind::BinaryOp},
        {"TypeRef", AstKind::TypeRef},
        {"ArrayType", AstKind::ArrayType},
        {"RangeType", AstKind::RangeType},
        {"EnumType", AstKind::EnumType},
        {"RecordType", AstKind::RecordType},
        {"FieldDecl", AstKind::FieldDecl}
    };

    const auto found = kinds.find(text);
    if (found == kinds.end()) {
        return false;
    }

    *out = found->second;
    return true;
}

bool parse_literal_kind(const std::string& text, LiteralKind* out) {
    static const std::unordered_map<std::string, LiteralKind> kinds = {
        {"None", LiteralKind::None},
        {"Integer", LiteralKind::Integer},
        {"Real", LiteralKind::Real},
        {"Char", LiteralKind::Char},
        {"String", LiteralKind::String},
        {"Boolean", LiteralKind::Boolean},
        {"IdentifierConstant", LiteralKind::IdentifierConstant}
    };

    const auto found = kinds.find(text);
    if (found == kinds.end()) {
        return false;
    }

    *out = found->second;
    return true;
}

int builtin_type_id(const std::string& type_name) {
    const std::string normalized = lower(type_name);
    if (normalized == "integer") return 1;
    if (normalized == "real") return 2;
    if (normalized == "char") return 3;
    if (normalized == "boolean") return 4;
    if (normalized == "string") return 5;
    return 0;
}

std::size_t next_metadata_position(const std::string& text, std::size_t start) {
    static const std::vector<std::string> keys = {
        " value=", " op=", " literal=", " : type=",
        ", tab_index=", " tab_index=",
        ", lev=", " lev=",
        ", block=", " block=",
        ", init=", " init="
    };

    std::size_t best = std::string::npos;
    for (const auto& key : keys) {
        const std::size_t found = text.find(key, start);
        if (found != std::string::npos && (best == std::string::npos || found < best)) {
            best = found;
        }
    }
    return best;
}

bool parse_int_metadata(const std::string& label,
                        const std::string& key,
                        int* out,
                        std::string* diagnostic) {
    const std::size_t key_pos = label.find(key);
    if (key_pos == std::string::npos) {
        return true;
    }

    const std::size_t value_start = key_pos + key.size();
    const std::size_t value_end = next_metadata_position(label, value_start);
    const std::string value = trim(label.substr(value_start,
        value_end == std::string::npos ? std::string::npos : value_end - value_start));

    try {
        *out = std::stoi(value);
        return true;
    } catch (const std::exception&) {
        *diagnostic = "Invalid integer metadata '" + key + value + "'.";
        return false;
    }
}

std::string parse_string_metadata(const std::string& label, const std::string& key) {
    const std::size_t key_pos = label.find(key);
    if (key_pos == std::string::npos) {
        return "";
    }

    const std::size_t value_start = key_pos + key.size();
    const std::size_t value_end = next_metadata_position(label, value_start);
    return trim(label.substr(value_start,
        value_end == std::string::npos ? std::string::npos : value_end - value_start));
}

struct ParsedLine {
    int depth = 0;
    std::string label;
};

ParsedLine strip_tree_prefix(const std::string& raw_line) {
    ParsedLine parsed;
    const std::string line = trim(raw_line);
    const std::size_t pipe_connector = raw_line.find("|-- ");
    const std::size_t last_connector = raw_line.find("\\-- ");

    std::size_t connector = std::string::npos;
    if (pipe_connector != std::string::npos) connector = pipe_connector;
    if (last_connector != std::string::npos &&
        (connector == std::string::npos || last_connector < connector)) {
        connector = last_connector;
    }

    if (connector == std::string::npos) {
        parsed.depth = 0;
        parsed.label = line;
        return parsed;
    }

    parsed.depth = static_cast<int>(connector / 4) + 1;
    parsed.label = trim(raw_line.substr(connector + 4));
    return parsed;
}

AstNodePtr parse_node_label(const std::string& label,
                            int line_number,
                            std::vector<std::string>* diagnostics) {
    std::size_t kind_end = 0;
    while (kind_end < label.size() &&
           (std::isalnum(static_cast<unsigned char>(label[kind_end])) ||
            label[kind_end] == '_')) {
        ++kind_end;
    }

    if (kind_end == 0) {
        diagnostics->push_back("Line " + std::to_string(line_number) +
                               ": expected Decorated AST node label.");
        return nullptr;
    }

    const std::string kind_name = label.substr(0, kind_end);
    AstKind kind;
    if (!parse_ast_kind(kind_name, &kind)) {
        diagnostics->push_back("Line " + std::to_string(line_number) +
                               ": unknown AST node kind '" + kind_name + "'.");
        return nullptr;
    }

    auto node = make_ast(kind, SourceLocation{line_number, 1});
    std::size_t metadata_start = kind_end;
    if (metadata_start < label.size() && label[metadata_start] == '(') {
        const std::size_t name_end = label.find(')', metadata_start + 1);
        if (name_end == std::string::npos) {
            diagnostics->push_back("Line " + std::to_string(line_number) +
                                   ": missing ')' in node name.");
            return nullptr;
        }
        node->name = label.substr(metadata_start + 1, name_end - metadata_start - 1);
        metadata_start = name_end + 1;
    }

    const std::string metadata = label.substr(metadata_start);
    node->value = parse_string_metadata(metadata, "value=");
    node->op = parse_string_metadata(metadata, "op=");

    const std::string literal_name = parse_string_metadata(metadata, "literal=");
    if (!literal_name.empty() && !parse_literal_kind(literal_name, &node->literal_kind)) {
        diagnostics->push_back("Line " + std::to_string(line_number) +
                               ": unknown literal kind '" + literal_name + "'.");
    }

    const std::string type_name = parse_string_metadata(metadata, ": type=");
    node->annotation.type_id = builtin_type_id(type_name);

    std::string diagnostic;
    if (!parse_int_metadata(metadata, "tab_index=", &node->annotation.tab_index, &diagnostic) ||
        !parse_int_metadata(metadata, "lev=", &node->annotation.lexical_level, &diagnostic) ||
        !parse_int_metadata(metadata, "block=", &node->annotation.block_index, &diagnostic)) {
        diagnostics->push_back("Line " + std::to_string(line_number) + ": " + diagnostic);
    }

    const std::string init = lower(parse_string_metadata(metadata, "init="));
    if (!init.empty()) {
        node->annotation.initialized = init == "yes" || init == "true" || init == "1";
    }

    return node;
}

bool looks_like_arion_source(const std::string& line) {
    const std::string normalized = lower(trim(line));
    return normalized == "program" ||
           starts_with(normalized, "program ") ||
           starts_with(normalized, "program\t");
}

}

bool DecoratedAstLoadResult::ok() const {
    return root && diagnostics.empty();
}

DecoratedAstLoadResult load_decorated_ast_file(const std::string& path) {
    DecoratedAstLoadResult result;
    std::ifstream in(path);
    if (!in.is_open()) {
        result.diagnostics.push_back("Failed to open Decorated AST input file: " + path);
        return result;
    }

    std::vector<AstNodePtr> stack;
    bool started = false;
    bool skipped_section = false;
    std::string line;
    int line_number = 0;

    while (std::getline(in, line)) {
        ++line_number;
        const std::string stripped = trim(line);
        if (stripped.empty()) {
            continue;
        }

        if (!started && stripped == "=== Decorated AST ===") {
            started = true;
            continue;
        }

        if (!started && starts_with(stripped, "===")) {
            skipped_section = true;
            continue;
        }

        if (!started && skipped_section) {
            continue;
        }

        if (!started) {
            if (looks_like_arion_source(stripped)) {
                result.diagnostics.push_back(
                    "Milestone 4 input must be Decorated AST, not Arion source code.");
                return result;
            }
            started = true;
        }

        if (started && starts_with(stripped, "===")) {
            break;
        }

        if (stripped == "<empty>") {
            result.diagnostics.push_back("Decorated AST input is empty.");
            return result;
        }

        const ParsedLine parsed = strip_tree_prefix(line);
        auto node = parse_node_label(parsed.label, line_number, &result.diagnostics);
        if (!node) {
            continue;
        }

        if (parsed.depth == 0) {
            if (result.root) {
                result.diagnostics.push_back("Line " + std::to_string(line_number) +
                                             ": multiple Decorated AST root nodes.");
                continue;
            }
            result.root = node;
        } else {
            if (!result.root) {
                result.diagnostics.push_back("Line " + std::to_string(line_number) +
                                             ": child node appears before root node.");
                continue;
            }
            if (parsed.depth > static_cast<int>(stack.size())) {
                result.diagnostics.push_back("Line " + std::to_string(line_number) +
                                             ": tree indentation skips a parent level.");
                continue;
            }
            stack[static_cast<std::size_t>(parsed.depth - 1)]->add_child(node);
        }

        if (parsed.depth >= static_cast<int>(stack.size())) {
            stack.resize(static_cast<std::size_t>(parsed.depth + 1));
        }
        stack[static_cast<std::size_t>(parsed.depth)] = node;
    }

    if (!result.root && result.diagnostics.empty()) {
        result.diagnostics.push_back(skipped_section
            ? "Decorated AST section was not found."
            : "Decorated AST input is empty.");
    }

    return result;
}

}
