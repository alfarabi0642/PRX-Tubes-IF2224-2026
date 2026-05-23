#include "parser.hpp"

#include <string>

namespace {
bool is_expression_start(TokenType type) {
    return type == TokenType::TOKEN_PLUS ||
           type == TokenType::TOKEN_MINUS ||
           type == TokenType::TOKEN_IDENT ||
           type == TokenType::TOKEN_INTCON ||
           type == TokenType::TOKEN_REALCON ||
           type == TokenType::TOKEN_CHARCON ||
           type == TokenType::TOKEN_STRING ||
           type == TokenType::TOKEN_LPARENT ||
           type == TokenType::TOKEN_NOTSY;
}

bool is_statement_boundary(TokenType type) {
    return type == TokenType::TOKEN_SEMICOLON ||
           type == TokenType::TOKEN_ENDSY ||
           type == TokenType::TOKEN_ELSESY ||
           type == TokenType::TOKEN_UNTILSY ||
           type == TokenType::TOKEN_PERIOD ||
           type == TokenType::TOKEN_EOF;
}

void add_child_or_placeholder(const std::shared_ptr<ParseTreeNode>& parent,
                              const std::shared_ptr<ParseTreeNode>& child,
                              const std::string& placeholder) {
    parent->add_child(child ? child : make_node(placeholder));
}
}

std::shared_ptr<ParseTreeNode> Parser::parse_subprogram_declaration() {
    auto node = make_node("<subprogram-declaration>");

    if (check(TokenType::TOKEN_PROCEDURESY)) {
        node->add_child(parse_procedure_declaration());
    } else if (check(TokenType::TOKEN_FUNCTIONSY)) {
        node->add_child(parse_function_declaration());
    } else {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected proceduresy or functionsy");
        node->add_child(make_node("<error>"));
        synchronize();
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_procedure_declaration() {
    auto node = make_node("<procedure-declaration>");

    node->add_child(terminal(expect(TokenType::TOKEN_PROCEDURESY)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));

    if (check(TokenType::TOKEN_LPARENT)) {
        node->add_child(parse_formal_parameter_list());
    }

    node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));
    node->add_child(parse_block());
    node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_function_declaration() {
    auto node = make_node("<function-declaration>");

    node->add_child(terminal(expect(TokenType::TOKEN_FUNCTIONSY)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));

    if (check(TokenType::TOKEN_LPARENT)) {
        node->add_child(parse_formal_parameter_list());
    }

    node->add_child(terminal(expect(TokenType::TOKEN_COLON)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));
    node->add_child(parse_block());
    node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_formal_parameter_list() {
    auto node = make_node("<formal-parameter-list>");

    node->add_child(terminal(expect(TokenType::TOKEN_LPARENT)));

    if (!check(TokenType::TOKEN_RPARENT)) {
        node->add_child(parse_parameter_group());

        while (check(TokenType::TOKEN_SEMICOLON)) {
            node->add_child(terminal(advance()));

            if (check(TokenType::TOKEN_RPARENT)) {
                Token token = current_token();
                error("Syntax error at line " + std::to_string(token.get_line()) +
                      ", col " + std::to_string(token.get_column()) +
                      ": unexpected token " + Token::get_type_name(token.get_type()) +
                      ", expected parameter group");
                node->add_child(make_node("<error>"));
                break;
            }

            node->add_child(parse_parameter_group());
        }
    }

    node->add_child(terminal(expect(TokenType::TOKEN_RPARENT)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_parameter_group() {
    auto node = make_node("<parameter-group>");

    add_child_or_placeholder(node, parse_identifier_list(), "<identifier-list>");
    node->add_child(terminal(expect(TokenType::TOKEN_COLON)));

    if (check(TokenType::TOKEN_IDENT)) {
        node->add_child(terminal(advance()));
    } else if (check(TokenType::TOKEN_ARRAYSY)) {
        add_child_or_placeholder(node, parse_array_type(), "<array-type>");
    } else {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected ident or arraysy");
        node->add_child(make_node("<error>"));

        if (!check(TokenType::TOKEN_SEMICOLON) &&
            !check(TokenType::TOKEN_RPARENT) &&
            !is_at_end()) {
            advance();
        }
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_statement() {
    auto node = make_node("<statement>");

    if (check(TokenType::TOKEN_IFSY)) {
        node->add_child(parse_if_statement());
    } else if (check(TokenType::TOKEN_CASESY)) {
        add_child_or_placeholder(node, parse_case_statement(), "<case-statement>");
    } else if (check(TokenType::TOKEN_WHILESY)) {
        add_child_or_placeholder(node, parse_while_statement(), "<while-statement>");
    } else if (check(TokenType::TOKEN_REPEATSY)) {
        add_child_or_placeholder(node, parse_repeat_statement(), "<repeat-statement>");
    } else if (check(TokenType::TOKEN_FORSY)) {
        add_child_or_placeholder(node, parse_for_statement(), "<for-statement>");
    } else if (check(TokenType::TOKEN_BEGINSY)) {
        node->add_child(parse_compound_statement());
    } else if (check(TokenType::TOKEN_IDENT)) {
        if (peek(1).get_type() == TokenType::TOKEN_LPARENT) {
            add_child_or_placeholder(node, parse_procedure_function_call(),
                                     "<procedure/function-call>");
        } else {
            size_t offset = 1;

            while (true) {
                TokenType type = peek(static_cast<int>(offset)).get_type();

                if (type == TokenType::TOKEN_LBRACK) {
                    int depth = 0;

                    do {
                        TokenType bracket_type = peek(static_cast<int>(offset)).get_type();

                        if (bracket_type == TokenType::TOKEN_LBRACK) {
                            ++depth;
                        } else if (bracket_type == TokenType::TOKEN_RBRACK) {
                            --depth;
                        }

                        ++offset;

                        if (bracket_type == TokenType::TOKEN_EOF ||
                            (depth > 0 && is_statement_boundary(bracket_type))) {
                            break;
                        }
                    } while (depth > 0);
                } else if (type == TokenType::TOKEN_PERIOD &&
                           peek(static_cast<int>(offset + 1)).get_type() == TokenType::TOKEN_IDENT) {
                    offset += 2;
                } else {
                    break;
                }
            }

            if (peek(static_cast<int>(offset)).get_type() == TokenType::TOKEN_BECOMES) {
                node->add_child(parse_assignment_statement());
            } else {
                node->add_child(parse_variable());
            }
        }
    } else if (!is_statement_boundary(current_token().get_type())) {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected statement");
        node->add_child(make_node("<error>"));
        advance();
        synchronize();
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_variable() {
    auto node = make_node("<variable>");

    if (check(TokenType::TOKEN_IDENT)) {
        node->add_child(terminal(advance()));
    } else {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected ident");
        node->add_child(make_node("<error>"));

        if (!is_at_end()) {
            advance();
        }

        return node;
    }

    while (check(TokenType::TOKEN_LBRACK) || check(TokenType::TOKEN_PERIOD)) {
        node->add_child(parse_component_variable());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_component_variable() {
    auto node = make_node("<component-variable>");

    if (check(TokenType::TOKEN_LBRACK)) {
        node->add_child(terminal(advance()));
        node->add_child(parse_index_list());
        node->add_child(terminal(expect(TokenType::TOKEN_RBRACK)));
    } else if (check(TokenType::TOKEN_PERIOD)) {
        node->add_child(terminal(advance()));
        node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    } else {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected lbrack or period");
        node->add_child(make_node("<error>"));
        synchronize();
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_index_list() {
    auto node = make_node("<index-list>");

    auto parse_index_expression = [&]() {
        if (is_expression_start(current_token().get_type())) {
            node->add_child(parse_expression());
            return;
        }

        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              ", expected expression");
        node->add_child(make_node("<error>"));

        if (!check(TokenType::TOKEN_COMMA) &&
            !check(TokenType::TOKEN_RBRACK) &&
            !is_at_end()) {
            advance();
        }
    };

    parse_index_expression();

    while (check(TokenType::TOKEN_COMMA)) {
        node->add_child(terminal(advance()));
        parse_index_expression();
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_assignment_statement() {
    auto node = make_node("<assignment-statement>");

    node->add_child(parse_variable());
    node->add_child(terminal(expect(TokenType::TOKEN_BECOMES)));
    add_child_or_placeholder(node, parse_expression(), "<expression>");

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_if_statement() {
    auto node = make_node("<if-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_IFSY)));
    add_child_or_placeholder(node, parse_expression(), "<expression>");
    node->add_child(terminal(expect(TokenType::TOKEN_THENSY)));
    node->add_child(parse_statement());

    if (check(TokenType::TOKEN_ELSESY)) {
        node->add_child(terminal(advance()));
        node->add_child(parse_statement());
    }

    return node;
}

