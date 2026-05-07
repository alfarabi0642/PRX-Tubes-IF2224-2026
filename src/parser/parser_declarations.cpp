#include "parser.hpp"

namespace {

bool is_sign(TokenType type) {
    return type == TokenType::TOKEN_PLUS ||
           type == TokenType::TOKEN_MINUS;
}

bool is_literal_constant(TokenType type) {
    return type == TokenType::TOKEN_CHARCON ||
           type == TokenType::TOKEN_STRING;
}

bool is_signed_constant_value(TokenType type) {
    return type == TokenType::TOKEN_IDENT ||
           type == TokenType::TOKEN_INTCON ||
           type == TokenType::TOKEN_REALCON;
}

bool starts_constant(TokenType type) {
    return is_literal_constant(type) ||
           is_sign(type) ||
           is_signed_constant_value(type);
}

std::string syntax_error_at(const Token& token, const std::string& expected) {
    return "Syntax error at line " + std::to_string(token.get_line()) +
           ", col " + std::to_string(token.get_column()) +
           ": unexpected token " + Token::get_type_name(token.get_type()) +
           ", expected " + expected;
}

} 

std::shared_ptr<ParseTreeNode> Parser::parse_const_declaration() {
    auto node = make_node("<const-declaration>");

    node->add_child(terminal(expect(TokenType::TOKEN_CONSTSY)));

    if (!check(TokenType::TOKEN_IDENT)) {
        error("Syntax error: const declaration requires at least one identifier");
        return node;
    }

    while (check(TokenType::TOKEN_IDENT)) {
        node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
        node->add_child(terminal(expect(TokenType::TOKEN_EQL)));
        node->add_child(parse_constant());
        node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_constant() {
    auto node = make_node("<constant>");
    const TokenType type = current_token().get_type();

    if (is_literal_constant(type)) {
        node->add_child(terminal(advance()));
        return node;
    }

    if (is_sign(type)) {
        node->add_child(terminal(advance()));

        if (!is_signed_constant_value(current_token().get_type())) {
            error(syntax_error_at(current_token(), "ident, intcon, or realcon after sign in constant"));
            return node;
        }

        node->add_child(terminal(advance()));
        return node;
    }

    if (is_signed_constant_value(type)) {
        node->add_child(terminal(advance()));
        return node;
    }

    error(syntax_error_at(current_token(), "constant"));
    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_type_declaration() {
    auto node = make_node("<type-declaration>");

    node->add_child(terminal(expect(TokenType::TOKEN_TYPESY)));

    if (!check(TokenType::TOKEN_IDENT)) {
        error("Syntax error: type declaration requires at least one identifier");
        return node;
    }

    while (check(TokenType::TOKEN_IDENT)) {
        node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
        node->add_child(terminal(expect(TokenType::TOKEN_EQL)));
        node->add_child(parse_type());
        node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_var_declaration() {
    auto node = make_node("<var-declaration>");

    node->add_child(terminal(expect(TokenType::TOKEN_VARSY)));

    if (!check(TokenType::TOKEN_IDENT)) {
        error("Syntax error: var declaration requires at least one identifier");
        return node;
    }

    while (check(TokenType::TOKEN_IDENT)) {
        node->add_child(parse_identifier_list());
        node->add_child(terminal(expect(TokenType::TOKEN_COLON)));
        node->add_child(parse_type());
        node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_identifier_list() {
    auto node = make_node("<identifier-list>");

    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));

    while (check(TokenType::TOKEN_COMMA)) {
        node->add_child(terminal(advance()));
        node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_type() {
    auto node = make_node("<type>");
    const TokenType type = current_token().get_type();

    if (type == TokenType::TOKEN_ARRAYSY) {
        node->add_child(parse_array_type());
        return node;
    }

    if (type == TokenType::TOKEN_LPARENT) {
        node->add_child(parse_enumerated());
        return node;
    }

    if (type == TokenType::TOKEN_RECORDSY) {
        node->add_child(parse_record_type());
        return node;
    }


    if (starts_constant(type)) {
        const int next_after_constant = is_sign(type) ? 2 : 1;
        if (peek(next_after_constant).get_type() == TokenType::TOKEN_PERIOD &&
            peek(next_after_constant + 1).get_type() == TokenType::TOKEN_PERIOD) {
            node->add_child(parse_range());
            return node;
        }
    }

    if (type == TokenType::TOKEN_IDENT) {
        node->add_child(terminal(advance()));
        return node;
    }

    error(syntax_error_at(current_token(), "type"));
    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_array_type() {
    auto node = make_node("<array-type>");

    node->add_child(terminal(expect(TokenType::TOKEN_ARRAYSY)));
    node->add_child(terminal(expect(TokenType::TOKEN_LBRACK)));

    if (check(TokenType::TOKEN_IDENT) && peek(1).get_type() != TokenType::TOKEN_PERIOD) {
        node->add_child(terminal(advance()));
    } else {
        node->add_child(parse_range());
    }

    node->add_child(terminal(expect(TokenType::TOKEN_RBRACK)));
    node->add_child(terminal(expect(TokenType::TOKEN_OFSY)));
    node->add_child(parse_type());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_range() {
    auto node = make_node("<range>");

    node->add_child(parse_constant());
    node->add_child(terminal(expect(TokenType::TOKEN_PERIOD)));
    node->add_child(terminal(expect(TokenType::TOKEN_PERIOD)));
    node->add_child(parse_constant());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_enumerated() {
    auto node = make_node("<enumerated>");

    node->add_child(terminal(expect(TokenType::TOKEN_LPARENT)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));

    while (check(TokenType::TOKEN_COMMA)) {
        node->add_child(terminal(advance()));
        node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    }

    node->add_child(terminal(expect(TokenType::TOKEN_RPARENT)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_record_type() {
    auto node = make_node("<record-type>");

    node->add_child(terminal(expect(TokenType::TOKEN_RECORDSY)));
    node->add_child(parse_field_list());
    node->add_child(terminal(expect(TokenType::TOKEN_ENDSY)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_field_list() {
    auto node = make_node("<field-list>");

    node->add_child(parse_field_part());

    while (check(TokenType::TOKEN_SEMICOLON)) {
        node->add_child(terminal(advance()));
        if (check(TokenType::TOKEN_ENDSY)) {
            break;
        }
        node->add_child(parse_field_part());
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_field_part() {
    auto node = make_node("<field-part>");

    node->add_child(parse_identifier_list());
    node->add_child(terminal(expect(TokenType::TOKEN_COLON)));
    node->add_child(parse_type());

    return node;
}
