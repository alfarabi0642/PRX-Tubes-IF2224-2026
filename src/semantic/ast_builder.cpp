#include "ast_builder.hpp"

#include <string>
#include <vector>

namespace semantic {

namespace {

bool is_node(const std::shared_ptr<ParseTreeNode>& node, const std::string& name) {
    return node && node->name == name;
}

std::string terminal_token(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return "";
    if (node->token.has_value()) return Token::get_type_name(node->token->get_type());
    const auto pos = node->name.find('(');
    return pos == std::string::npos ? node->name : node->name.substr(0, pos);
}

bool terminal_is(const std::shared_ptr<ParseTreeNode>& node, const std::string& token_name) {
    return terminal_token(node) == token_name;
}

std::string terminal_value(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return "";
    if (node->token.has_value()) return node->token->get_value();
    const auto open = node->name.find('(');
    const auto close = node->name.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close <= open) return "";
    return node->name.substr(open + 1, close - open - 1);
}

SourceLocation location_from_node(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return {};
    if (node->token.has_value()) {
        return {node->token->get_line(), node->token->get_column()};
    }
    for (const auto& child : node->children) {
        SourceLocation location = location_from_node(child);
        if (location.line != 0 || location.column != 0) return location;
    }
    return {};
}

std::shared_ptr<ParseTreeNode> first_child(const std::shared_ptr<ParseTreeNode>& node,
                                           const std::string& name) {
    if (!node) return nullptr;
    for (const auto& child : node->children) {
        if (is_node(child, name)) return child;
    }
    return nullptr;
}

std::vector<std::shared_ptr<ParseTreeNode>> children_named(const std::shared_ptr<ParseTreeNode>& node,
                                                           const std::string& name) {
    std::vector<std::shared_ptr<ParseTreeNode>> result;
    if (!node) return result;
    for (const auto& child : node->children) {
        if (is_node(child, name)) result.push_back(child);
    }
    return result;
}

AstNodePtr literal_from_terminal(const std::shared_ptr<ParseTreeNode>& node, bool identifier_constant) {
    auto ast = make_ast(AstKind::Literal, location_from_node(node));
    const std::string token = terminal_token(node);
    ast->value = terminal_value(node);
    ast->name = ast->value;

    if (token == "intcon") ast->literal_kind = LiteralKind::Integer;
    else if (token == "realcon") ast->literal_kind = LiteralKind::Real;
    else if (token == "charcon") ast->literal_kind = LiteralKind::Char;
    else if (token == "string") ast->literal_kind = LiteralKind::String;
    else if (token == "ident" && identifier_constant) ast->literal_kind = LiteralKind::IdentifierConstant;
    else if (token == "ident") {
        ast->kind = AstKind::Variable;
        ast->literal_kind = LiteralKind::None;
    }

    return ast;
}

std::vector<std::string> collect_identifiers(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<std::string> ids;
    if (!node) return ids;
    for (const auto& child : node->children) {
        if (terminal_is(child, "ident")) ids.push_back(terminal_value(child));
    }
    return ids;
}

AstNodePtr build_program(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_declaration_part(const std::shared_ptr<ParseTreeNode>& node);
std::vector<AstNodePtr> build_const_declaration(const std::shared_ptr<ParseTreeNode>& node);
std::vector<AstNodePtr> build_type_declaration(const std::shared_ptr<ParseTreeNode>& node);
std::vector<AstNodePtr> build_var_declaration(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_subprogram_declaration(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_compound_statement(const std::shared_ptr<ParseTreeNode>& node);
std::vector<AstNodePtr> build_statement_list(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_statement(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_type_node(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_constant(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_expression(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_simple_expression(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_term(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_factor(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_variable(const std::shared_ptr<ParseTreeNode>& node);
AstNodePtr build_call(const std::shared_ptr<ParseTreeNode>& node);

AstNodePtr build_program(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = make_ast(AstKind::Program, location_from_node(node));
    auto header = first_child(node, "<program-header>");
    if (header) {
        for (const auto& child : header->children) {
            if (terminal_is(child, "ident")) {
                ast->name = terminal_value(child);
                break;
            }
        }
    }
    ast->add_child(build_declaration_part(first_child(node, "<declaration-part>")));
    ast->add_child(build_compound_statement(first_child(node, "<compound-statement>")));
    return ast;
}

AstNodePtr build_declaration_part(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = make_ast(AstKind::DeclarationPart, location_from_node(node));
    if (!node) return ast;
    for (const auto& child : node->children) {
        if (is_node(child, "<const-declaration>")) {
            for (auto& decl : build_const_declaration(child)) ast->add_child(decl);
        } else if (is_node(child, "<type-declaration>")) {
            for (auto& decl : build_type_declaration(child)) ast->add_child(decl);
        } else if (is_node(child, "<var-declaration>")) {
            for (auto& decl : build_var_declaration(child)) ast->add_child(decl);
        } else if (is_node(child, "<subprogram-declaration>")) {
            ast->add_child(build_subprogram_declaration(child));
        }
    }
    return ast;
}

std::vector<AstNodePtr> build_const_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<AstNodePtr> declarations;
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!terminal_is(node->children[i], "ident")) continue;
        auto decl = make_ast(AstKind::ConstDecl, location_from_node(node->children[i]));
        decl->name = terminal_value(node->children[i]);
        if (i + 2 < node->children.size() && is_node(node->children[i + 2], "<constant>")) {
            decl->add_child(build_constant(node->children[i + 2]));
        }
        declarations.push_back(decl);
    }
    return declarations;
}

std::vector<AstNodePtr> build_type_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<AstNodePtr> declarations;
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!terminal_is(node->children[i], "ident")) continue;
        auto decl = make_ast(AstKind::TypeDecl, location_from_node(node->children[i]));
        decl->name = terminal_value(node->children[i]);
        if (i + 2 < node->children.size() && is_node(node->children[i + 2], "<type>")) {
            decl->add_child(build_type_node(node->children[i + 2]));
        }
        declarations.push_back(decl);
    }
    return declarations;
}

std::vector<AstNodePtr> build_var_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<AstNodePtr> declarations;
    for (size_t i = 0; node && i < node->children.size(); ++i) {
        if (!is_node(node->children[i], "<identifier-list>")) continue;
        auto ids = collect_identifiers(node->children[i]);
        auto type_node = (i + 2 < node->children.size() && is_node(node->children[i + 2], "<type>"))
                             ? build_type_node(node->children[i + 2])
                             : nullptr;
        for (const auto& id : ids) {
            auto decl = make_ast(AstKind::VarDecl, location_from_node(node->children[i]));
            decl->name = id;
            decl->add_child(type_node);
            declarations.push_back(decl);
        }
    }
    return declarations;
}

AstNodePtr build_formal_parameter_list(const std::shared_ptr<ParseTreeNode>& node) {
    auto params = make_ast(AstKind::ParameterGroup, location_from_node(node));
    if (!node) return params;
    for (const auto& group : children_named(node, "<parameter-group>")) {
        auto ids = collect_identifiers(first_child(group, "<identifier-list>"));
        AstNodePtr type_node;
        for (const auto& child : group->children) {
            if (terminal_is(child, "ident")) {
                type_node = make_ast(AstKind::TypeRef, location_from_node(child));
                type_node->name = terminal_value(child);
            } else if (is_node(child, "<array-type>")) {
                type_node = build_type_node(child);
            }
        }
        for (const auto& id : ids) {
            auto param = make_ast(AstKind::VarDecl, location_from_node(group));
            param->name = id;
            param->add_child(type_node);
            params->add_child(param);
        }
    }
    return params;
}

AstNodePtr build_subprogram_declaration(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node || node->children.empty()) return nullptr;
    const auto& decl_node = node->children.front();
    const bool is_function = is_node(decl_node, "<function-declaration>");
    auto ast = make_ast(is_function ? AstKind::FunctionDecl : AstKind::ProcedureDecl,
                        location_from_node(decl_node));
    AstNodePtr return_type;
    std::vector<AstNodePtr> parameter_groups;
    AstNodePtr declaration_part;
    AstNodePtr body;

    int ident_seen = 0;
    for (const auto& child : decl_node->children) {
        if (terminal_is(child, "ident")) {
            ++ident_seen;
            if (ident_seen == 1) ast->name = terminal_value(child);
            else if (is_function && ident_seen == 2) {
                return_type = make_ast(AstKind::TypeRef, location_from_node(child));
                return_type->name = terminal_value(child);
            }
        } else if (is_node(child, "<formal-parameter-list>")) {
            parameter_groups.push_back(build_formal_parameter_list(child));
        } else if (is_node(child, "<block>")) {
            declaration_part = build_declaration_part(first_child(child, "<declaration-part>"));
            body = build_compound_statement(first_child(child, "<compound-statement>"));
        }
    }

    if (return_type) ast->add_child(return_type);
    for (const auto& parameter_group : parameter_groups) ast->add_child(parameter_group);
    ast->add_child(declaration_part);
    ast->add_child(body);

    return ast;
}

AstNodePtr build_type_node(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return nullptr;
    if (is_node(node, "<type>") && !node->children.empty()) return build_type_node(node->children.front());

    if (terminal_is(node, "ident")) {
        auto ast = make_ast(AstKind::TypeRef, location_from_node(node));
        ast->name = terminal_value(node);
        return ast;
    }

    if (is_node(node, "<range>")) {
        auto ast = make_ast(AstKind::RangeType, location_from_node(node));
        for (const auto& child : node->children) {
            if (is_node(child, "<constant>")) ast->add_child(build_constant(child));
        }
        return ast;
    }

    if (is_node(node, "<enumerated>")) {
        auto ast = make_ast(AstKind::EnumType, location_from_node(node));
        for (const auto& child : node->children) {
            if (terminal_is(child, "ident")) ast->add_child(literal_from_terminal(child, true));
        }
        return ast;
    }

    if (is_node(node, "<array-type>")) {
        auto ast = make_ast(AstKind::ArrayType, location_from_node(node));
        bool after_of = false;
        for (const auto& child : node->children) {
            if (terminal_is(child, "ofsy")) {
                after_of = true;
            } else if (!after_of && (is_node(child, "<range>") || terminal_is(child, "ident"))) {
                ast->add_child(build_type_node(child));
            } else if (after_of && (is_node(child, "<type>") || terminal_is(child, "ident") ||
                                    is_node(child, "<array-type>") || is_node(child, "<record-type>"))) {
                ast->add_child(build_type_node(child));
            }
        }
        return ast;
    }

    if (is_node(node, "<record-type>")) {
        auto ast = make_ast(AstKind::RecordType, location_from_node(node));
        auto fields = first_child(node, "<field-list>");
        if (fields) {
            for (const auto& field_part : children_named(fields, "<field-part>")) {
                auto ids = collect_identifiers(first_child(field_part, "<identifier-list>"));
                AstNodePtr field_type = build_type_node(first_child(field_part, "<type>"));
                for (const auto& id : ids) {
                    auto field = make_ast(AstKind::FieldDecl, location_from_node(field_part));
                    field->name = id;
                    field->add_child(field_type);
                    ast->add_child(field);
                }
            }
        }
        return ast;
    }

    return nullptr;
}

AstNodePtr build_constant(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return nullptr;
    std::string sign;
    std::shared_ptr<ParseTreeNode> value_node;
    for (const auto& child : node->children) {
        if (terminal_is(child, "plus") || terminal_is(child, "minus")) sign = terminal_token(child);
        else if (terminal_is(child, "ident") || terminal_is(child, "intcon") ||
                 terminal_is(child, "realcon") || terminal_is(child, "charcon") ||
                 terminal_is(child, "string")) {
            value_node = child;
        }
    }
    auto literal = literal_from_terminal(value_node, true);
    if (!sign.empty() && literal) {
        auto unary = make_ast(AstKind::UnaryOp, location_from_node(node));
        unary->op = sign;
        unary->add_child(literal);
        return unary;
    }
    return literal;
}

AstNodePtr build_compound_statement(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = make_ast(AstKind::CompoundStatement, location_from_node(node));
    for (auto& statement : build_statement_list(first_child(node, "<statement-list>"))) {
        ast->add_child(statement);
    }
    return ast;
}

std::vector<AstNodePtr> build_statement_list(const std::shared_ptr<ParseTreeNode>& node) {
    std::vector<AstNodePtr> statements;
    if (!node) return statements;
    for (const auto& child : node->children) {
        if (is_node(child, "<statement>")) statements.push_back(build_statement(child));
    }
    return statements;
}

AstNodePtr build_case_branch(const std::shared_ptr<ParseTreeNode>& node) {
    auto branch = make_ast(AstKind::CaseBranch, location_from_node(node));
    for (const auto& child : node ? node->children : std::vector<std::shared_ptr<ParseTreeNode>>{}) {
        if (is_node(child, "<constant>")) branch->add_child(build_constant(child));
        else if (is_node(child, "<statement>")) branch->add_child(build_statement(child));
    }
    return branch;
}

void append_case_branches(const std::shared_ptr<ParseTreeNode>& node, const AstNodePtr& parent) {
    if (!node) return;
    parent->add_child(build_case_branch(node));
    for (const auto& child : node->children) {
        if (is_node(child, "<case-block>")) append_case_branches(child, parent);
    }
}

AstNodePtr build_statement(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node || node->children.empty()) return make_ast(AstKind::EmptyStatement, location_from_node(node));
    const auto& child = node->children.front();

    if (is_node(child, "<assignment-statement>")) {
        auto ast = make_ast(AstKind::AssignStatement, location_from_node(child));
        ast->add_child(build_variable(first_child(child, "<variable>")));
        ast->add_child(build_expression(first_child(child, "<expression>")));
        return ast;
    }
    if (is_node(child, "<if-statement>")) {
        auto ast = make_ast(AstKind::IfStatement, location_from_node(child));
        ast->add_child(build_expression(first_child(child, "<expression>")));
        for (const auto& item : child->children) {
            if (is_node(item, "<statement>")) ast->add_child(build_statement(item));
        }
        return ast;
    }
    if (is_node(child, "<while-statement>")) {
        auto ast = make_ast(AstKind::WhileStatement, location_from_node(child));
        ast->add_child(build_expression(first_child(child, "<expression>")));
        ast->add_child(build_compound_statement(first_child(child, "<compound-statement>")));
        return ast;
    }
    if (is_node(child, "<repeat-statement>")) {
        auto ast = make_ast(AstKind::RepeatStatement, location_from_node(child));
        auto body = make_ast(AstKind::CompoundStatement, location_from_node(child));
        for (auto& statement : build_statement_list(first_child(child, "<statement-list>"))) {
            body->add_child(statement);
        }
        ast->add_child(body);
        for (const auto& item : child->children) {
            if (is_node(item, "<expression>")) ast->add_child(build_expression(item));
        }
        return ast;
    }
    if (is_node(child, "<for-statement>")) {
        auto ast = make_ast(AstKind::ForStatement, location_from_node(child));
        for (const auto& item : child->children) {
            if (terminal_is(item, "ident") && ast->name.empty()) ast->name = terminal_value(item);
            else if (terminal_is(item, "tosy") || terminal_is(item, "downtosy")) ast->op = terminal_token(item);
            else if (is_node(item, "<expression>")) ast->add_child(build_expression(item));
            else if (is_node(item, "<compound-statement>")) ast->add_child(build_compound_statement(item));
        }
        return ast;
    }
    if (is_node(child, "<case-statement>")) {
        auto ast = make_ast(AstKind::CaseStatement, location_from_node(child));
        ast->add_child(build_expression(first_child(child, "<expression>")));
        append_case_branches(first_child(child, "<case-block>"), ast);
        return ast;
    }
    if (is_node(child, "<procedure/function-call>")) return build_call(child);
    if (is_node(child, "<compound-statement>")) return build_compound_statement(child);
    if (is_node(child, "<variable>")) return build_variable(child);
    return make_ast(AstKind::EmptyStatement, location_from_node(node));
}

AstNodePtr build_variable(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = make_ast(AstKind::Variable, location_from_node(node));
    if (!node) return ast;
    for (const auto& child : node->children) {
        if (terminal_is(child, "ident") && ast->name.empty()) {
            ast->name = terminal_value(child);
        } else if (is_node(child, "<component-variable>")) {
            if (!child->children.empty() && terminal_is(child->children.front(), "period")) {
                auto field = make_ast(AstKind::FieldComponent, location_from_node(child));
                for (const auto& item : child->children) {
                    if (terminal_is(item, "ident")) field->name = terminal_value(item);
                }
                ast->add_child(field);
            } else {
                auto index = make_ast(AstKind::IndexComponent, location_from_node(child));
                std::vector<std::shared_ptr<ParseTreeNode>> stack;
                if (auto list = first_child(child, "<index-list>")) stack.push_back(list);
                while (!stack.empty()) {
                    auto current = stack.back();
                    stack.pop_back();
                    for (const auto& item : current->children) {
                        if (is_node(item, "<expression>")) {
                            index->add_child(build_expression(item));
                        } else if (is_node(item, "<index-list>")) {
                            stack.push_back(item);
                        }
                    }
                }
                ast->add_child(index);
            }
        }
    }
    return ast;
}

AstNodePtr build_call(const std::shared_ptr<ParseTreeNode>& node) {
    auto ast = make_ast(AstKind::Call, location_from_node(node));
    if (!node) return ast;
    for (const auto& child : node->children) {
        if (terminal_is(child, "ident") && ast->name.empty()) ast->name = terminal_value(child);
        else if (is_node(child, "<parameter-list>")) {
            for (const auto& item : child->children) {
                if (is_node(item, "<expression>")) ast->add_child(build_expression(item));
            }
        }
    }
    return ast;
}

std::string operator_token_from_node(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node || node->children.empty()) return terminal_token(node);
    return terminal_token(node->children.front());
}

AstNodePtr build_expression(const std::shared_ptr<ParseTreeNode>& node) {
    auto simple_exprs = children_named(node, "<simple-expression>");
    if (simple_exprs.empty()) return nullptr;
    AstNodePtr result = build_simple_expression(simple_exprs.front());
    if (simple_exprs.size() > 1) {
        auto binary = make_ast(AstKind::BinaryOp, location_from_node(node));
        binary->op = operator_token_from_node(first_child(node, "<relational-operator>"));
        binary->add_child(result);
        binary->add_child(build_simple_expression(simple_exprs[1]));
        result = binary;
    }
    return result;
}

AstNodePtr build_simple_expression(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return nullptr;
    std::vector<AstNodePtr> terms;
    std::vector<std::string> ops;
    std::string sign;
    for (const auto& child : node->children) {
        if ((terminal_is(child, "plus") || terminal_is(child, "minus")) && terms.empty()) sign = terminal_token(child);
        else if (is_node(child, "<term>")) terms.push_back(build_term(child));
        else if (is_node(child, "<additive-operator>")) ops.push_back(operator_token_from_node(child));
    }
    if (terms.empty()) return nullptr;
    AstNodePtr result = terms.front();
    if (!sign.empty() && sign == "minus") {
        auto unary = make_ast(AstKind::UnaryOp, location_from_node(node));
        unary->op = sign;
        unary->add_child(result);
        result = unary;
    }
    for (size_t i = 1; i < terms.size(); ++i) {
        auto binary = make_ast(AstKind::BinaryOp, location_from_node(node));
        binary->op = i - 1 < ops.size() ? ops[i - 1] : "";
        binary->add_child(result);
        binary->add_child(terms[i]);
        result = binary;
    }
    return result;
}

AstNodePtr build_term(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return nullptr;
    std::vector<AstNodePtr> factors;
    std::vector<std::string> ops;
    for (const auto& child : node->children) {
        if (is_node(child, "<factor>")) factors.push_back(build_factor(child));
        else if (is_node(child, "<multiplicative-operator>")) ops.push_back(operator_token_from_node(child));
    }
    if (factors.empty()) return nullptr;
    AstNodePtr result = factors.front();
    for (size_t i = 1; i < factors.size(); ++i) {
        auto binary = make_ast(AstKind::BinaryOp, location_from_node(node));
        binary->op = i - 1 < ops.size() ? ops[i - 1] : "";
        binary->add_child(result);
        binary->add_child(factors[i]);
        result = binary;
    }
    return result;
}

AstNodePtr build_factor(const std::shared_ptr<ParseTreeNode>& node) {
    if (!node) return nullptr;
    for (const auto& child : node->children) {
        if (is_node(child, "<variable>")) return build_variable(child);
        if (is_node(child, "<procedure/function-call>")) return build_call(child);
        if (is_node(child, "<expression>")) return build_expression(child);
        if (is_node(child, "<factor>")) {
            auto unary = make_ast(AstKind::UnaryOp, location_from_node(node));
            unary->op = "notsy";
            unary->add_child(build_factor(child));
            return unary;
        }
        if (terminal_is(child, "intcon") || terminal_is(child, "realcon") ||
            terminal_is(child, "charcon") || terminal_is(child, "string")) {
            return literal_from_terminal(child, false);
        }
    }
    return nullptr;
}

} // namespace

bool AstBuildResult::ok() const {
    return !has_errors(diagnostics) && root != nullptr;
}

AstBuildResult AstBuilder::build(const std::shared_ptr<ParseTreeNode>& parse_tree_root) {
    AstBuildResult result;
    if (!parse_tree_root) {
        result.diagnostics.push_back({DiagnosticSeverity::Error, {}, "AST builder: parse tree root is empty"});
        return result;
    }
    result.root = build_program(parse_tree_root);
    return result;
}

} // namespace semantic
