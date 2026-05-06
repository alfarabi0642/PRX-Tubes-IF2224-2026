#include "parser.hpp"
namespace {
bool is_constant_start(TokenType type) {
    return type == TokenType::TOKEN_CHARCON ||
           type == TokenType::TOKEN_STRING ||
           type == TokenType::TOKEN_PLUS ||
           type == TokenType::TOKEN_MINUS ||
           type == TokenType::TOKEN_IDENT ||
           type == TokenType::TOKEN_INTCON ||
           type == TokenType::TOKEN_REALCON;
}

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

bool is_relational_operator(TokenType type) {
    return type == TokenType::TOKEN_EQL ||
           type == TokenType::TOKEN_NEQ ||
           type == TokenType::TOKEN_GTR ||
           type == TokenType::TOKEN_GEQ ||
           type == TokenType::TOKEN_LSS ||
           type == TokenType::TOKEN_LEQ;
}

bool is_additive_operator(TokenType type) {
    return type == TokenType::TOKEN_PLUS ||
           type == TokenType::TOKEN_MINUS ||
           type == TokenType::TOKEN_ORSY;
}

bool is_multiplicative_operator(TokenType type) {
    return type == TokenType::TOKEN_TIMES ||
           type == TokenType::TOKEN_RDIV ||
           type == TokenType::TOKEN_IDIV ||
           type == TokenType::TOKEN_IMOD ||
           type == TokenType::TOKEN_ANDSY;
}

bool is_factor_recovery_token(TokenType type) {
    return type == TokenType::TOKEN_COMMA ||
           type == TokenType::TOKEN_SEMICOLON ||
           type == TokenType::TOKEN_COLON ||
           type == TokenType::TOKEN_RPARENT ||
           type == TokenType::TOKEN_RBRACK ||
           type == TokenType::TOKEN_ENDSY ||
           type == TokenType::TOKEN_ELSESY ||
           type == TokenType::TOKEN_UNTILSY ||
           type == TokenType::TOKEN_OFSY ||
           type == TokenType::TOKEN_DOSY ||
           type == TokenType::TOKEN_TOSY ||
           type == TokenType::TOKEN_DOWNTOSY ||
           type == TokenType::TOKEN_THENSY ||
           type == TokenType::TOKEN_PERIOD ||
           type == TokenType::TOKEN_EOF;
}

std::string expected_message(const Token& token, const std::string& expected) {
    return "Syntax error at line " + std::to_string(token.get_line()) +
           ", col " + std::to_string(token.get_column()) +
           ": unexpected token " + Token::get_type_name(token.get_type()) +
           ", expected " + expected;
}
}

std::shared_ptr<ParseTreeNode> Parser::parse_case_statement() {
    auto node = make_node("<case-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_CASESY)));
    node->add_child(parse_expression());
    node->add_child(terminal(expect(TokenType::TOKEN_OFSY)));
    node->add_child(parse_case_block());
    node->add_child(terminal(expect(TokenType::TOKEN_ENDSY)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_case_block() {
    auto node = make_node("<case-block>");

    if (!is_constant_start(current_token().get_type())) {
        error(expected_message(current_token(), "constant"));
        node->add_child(make_node("<error>"));
        return node;
    }

    node->add_child(parse_constant());
    while (check(TokenType::TOKEN_COMMA)) {
        node->add_child(terminal(advance()));

        if (!is_constant_start(current_token().get_type())) {
            error(expected_message(current_token(), "constant"));
            node->add_child(make_node("<error>"));
            break;
        }

        node->add_child(parse_constant());
    }

    node->add_child(terminal(expect(TokenType::TOKEN_COLON)));
    node->add_child(parse_statement());

    while (check(TokenType::TOKEN_SEMICOLON)) {
        node->add_child(terminal(advance()));

        if (check(TokenType::TOKEN_ENDSY) || is_at_end()) {
            break;
        }

        if (is_constant_start(current_token().get_type())) {
            node->add_child(parse_case_block());
            break;
        }

        error(expected_message(current_token(), "case label or endsy"));
        node->add_child(make_node("<error>"));
        if (!check(TokenType::TOKEN_SEMICOLON) && !is_at_end()) {
            advance();
        }
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_while_statement() {
    auto node = make_node("<while-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_WHILESY)));
    node->add_child(parse_expression());
    node->add_child(terminal(expect(TokenType::TOKEN_DOSY)));
    node->add_child(parse_statement());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_repeat_statement() {
    auto node = make_node("<repeat-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_REPEATSY)));
    node->add_child(parse_statement_list());
    node->add_child(terminal(expect(TokenType::TOKEN_UNTILSY)));
    node->add_child(parse_expression());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_for_statement() {
    auto node = make_node("<for-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_FORSY)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    node->add_child(terminal(expect(TokenType::TOKEN_BECOMES)));
    node->add_child(parse_expression());

    if (check(TokenType::TOKEN_TOSY) || check(TokenType::TOKEN_DOWNTOSY)) {
        node->add_child(terminal(advance()));
    } else {
        error(expected_message(current_token(), "tosy or downtosy"));
        node->add_child(make_node("<error>"));
    }

    node->add_child(parse_expression());
    node->add_child(terminal(expect(TokenType::TOKEN_DOSY)));
    node->add_child(parse_statement());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_procedure_function_call() {
    auto node = make_node("<procedure/function-call>");

    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    node->add_child(terminal(expect(TokenType::TOKEN_LPARENT)));

    if (check(TokenType::TOKEN_RPARENT)) {
        error(expected_message(current_token(), "<parameter-list>"));
        node->add_child(make_node("<parameter-list>"));
    } else {
        node->add_child(parse_parameter_list());
    }

    node->add_child(terminal(expect(TokenType::TOKEN_RPARENT)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_parameter_list() {
    auto node = make_node("<parameter-list>");

    if (!is_expression_start(current_token().get_type())) {
        error(expected_message(current_token(), "expression"));
        node->add_child(make_node("<error>"));
        if (!check(TokenType::TOKEN_RPARENT) && !is_at_end()) {
            node->add_child(terminal(advance()));
        }
        return node;
    }

    node->add_child(parse_expression());

    while (check(TokenType::TOKEN_COMMA)) {
        node->add_child(terminal(advance()));

        if (!is_expression_start(current_token().get_type())) {
            error(expected_message(current_token(), "expression"));
            node->add_child(make_node("<error>"));
            if (!check(TokenType::TOKEN_RPARENT) && !is_at_end()) {
                node->add_child(terminal(advance()));
            }
            break;
        }

        node->add_child(parse_expression());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_expression() {
    auto node = make_node("<expression>");

    node->add_child(parse_simple_expression());

    if (is_relational_operator(current_token().get_type())) {
        node->add_child(parse_relational_operator());
        node->add_child(parse_simple_expression());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_simple_expression() {
    auto node = make_node("<simple-expression>");

    if (check(TokenType::TOKEN_PLUS) || check(TokenType::TOKEN_MINUS)) {
        node->add_child(terminal(advance()));
    }

    node->add_child(parse_term());

    while (is_additive_operator(current_token().get_type())) {
        node->add_child(parse_additive_operator());
        node->add_child(parse_term());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_term() {
    auto node = make_node("<term>");

    node->add_child(parse_factor());

    while (is_multiplicative_operator(current_token().get_type())) {
        node->add_child(parse_multiplicative_operator());
        node->add_child(parse_factor());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_factor() {
    auto node = make_node("<factor>");
    Token token = current_token();

    if (token.get_type() == TokenType::TOKEN_IDENT) {
        if (peek(1).get_type() == TokenType::TOKEN_LPARENT) {
            node->add_child(parse_procedure_function_call());
        } else {
            node->add_child(parse_variable());
        }
    } else if (token.get_type() == TokenType::TOKEN_INTCON ||
               token.get_type() == TokenType::TOKEN_REALCON ||
               token.get_type() == TokenType::TOKEN_CHARCON ||
               token.get_type() == TokenType::TOKEN_STRING) {
        node->add_child(terminal(advance()));
    } else if (token.get_type() == TokenType::TOKEN_LPARENT) {
        node->add_child(terminal(advance()));
        node->add_child(parse_expression());
        node->add_child(terminal(expect(TokenType::TOKEN_RPARENT)));
    } else if (token.get_type() == TokenType::TOKEN_NOTSY) {
        node->add_child(terminal(advance()));
        node->add_child(parse_factor());
    } else {
        error(expected_message(token, "factor"));
        node->add_child(make_node("<error>"));
        if (!is_factor_recovery_token(token.get_type()) && !is_at_end()) {
            node->add_child(terminal(advance()));
        }
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_relational_operator() {
    auto node = make_node("<relational-operator>");

    if (is_relational_operator(current_token().get_type())) {
        node->add_child(terminal(advance()));
    } else {
        error(expected_message(current_token(), "relational operator"));
        node->add_child(make_node("<error>"));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_additive_operator() {
    auto node = make_node("<additive-operator>");

    if (is_additive_operator(current_token().get_type())) {
        node->add_child(terminal(advance()));
    } else {
        error(expected_message(current_token(), "additive operator"));
        node->add_child(make_node("<error>"));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_multiplicative_operator() {
    auto node = make_node("<multiplicative-operator>");

    if (is_multiplicative_operator(current_token().get_type())) {
        node->add_child(terminal(advance()));
    } else {
        error(expected_message(current_token(), "multiplicative operator"));
        node->add_child(make_node("<error>"));
    }

    return node;
}
