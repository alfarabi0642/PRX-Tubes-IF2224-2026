#include "parser.hpp"

std::shared_ptr<ParseTreeNode> Parser::parse_program() {
    auto node = make_node("<program>");

    node->add_child(parse_program_header());
    node->add_child(parse_declaration_part());
    node->add_child(parse_compound_statement());
    node->add_child(terminal(expect(TokenType::TOKEN_PERIOD)));

    if (!is_at_end()) {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": unexpected token " + Token::get_type_name(token.get_type()) +
              " after end of program");
    }
    

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_program_header() {
    auto node = make_node("<program-header>");

    node->add_child(terminal(expect(TokenType::TOKEN_PROGRAMSY)));
    node->add_child(terminal(expect(TokenType::TOKEN_IDENT)));
    node->add_child(terminal(expect(TokenType::TOKEN_SEMICOLON)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_declaration_part() {
    auto node = make_node("<declaration-part>");

    while (check(TokenType::TOKEN_CONSTSY)) {
        node->add_child(parse_const_declaration());
    }

    while (check(TokenType::TOKEN_TYPESY)) {
        node->add_child(parse_type_declaration());
    }

    while (check(TokenType::TOKEN_VARSY)) {
        node->add_child(parse_var_declaration());
    }

    while (check(TokenType::TOKEN_PROCEDURESY) || check(TokenType::TOKEN_FUNCTIONSY)) {
        node->add_child(parse_subprogram_declaration());
    }

    if (check(TokenType::TOKEN_CONSTSY) || check(TokenType::TOKEN_TYPESY) || check(TokenType::TOKEN_VARSY)) {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": declaration '" + Token::get_type_name(token.get_type()) +
              "' appears out of order (expected order: const, type, var, subprogram)");
    }

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_block() {
    auto node = make_node("<block>");

    node->add_child(parse_declaration_part());
    node->add_child(parse_compound_statement());

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_compound_statement() {
    auto node = make_node("<compound-statement>");

    node->add_child(terminal(expect(TokenType::TOKEN_BEGINSY)));
    node->add_child(parse_statement_list());
    node->add_child(terminal(expect(TokenType::TOKEN_ENDSY)));

    return node;
}

std::shared_ptr<ParseTreeNode> Parser::parse_statement_list() {
    auto node = make_node("<statement-list>");

    node->add_child(parse_statement());

    while (check(TokenType::TOKEN_SEMICOLON)) {
        node->add_child(terminal(advance()));
        node->add_child(parse_statement());
    }

    return node;
}
