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

    auto declaration_rank = [](TokenType type) -> int {
        if (type == TokenType::TOKEN_CONSTSY) return 0;
        if (type == TokenType::TOKEN_TYPESY) return 1;
        if (type == TokenType::TOKEN_VARSY) return 2;
        if (type == TokenType::TOKEN_PROCEDURESY || type == TokenType::TOKEN_FUNCTIONSY) return 3;
        return -1;
    };

    auto append_declaration = [&](const std::shared_ptr<ParseTreeNode>& child,
                                  const std::string& rule,
                                  size_t before) {
        node->add_child(child);
        if (pos == before && !is_at_end()) {
            Token token = current_token();
            error("Syntax error at line " + std::to_string(token.get_line()) +
                  ", col " + std::to_string(token.get_column()) +
                  ": " + rule + " did not consume token " +
                  Token::get_type_name(token.get_type()) +
                  "; advancing to continue parsing");
            advance();
        }
    };

    int current_rank = 0;
    while (!is_at_end()) {
        TokenType type = current_token().get_type();
        const int rank = declaration_rank(type);
        if (rank == -1) break;

        if (rank < current_rank) {
            Token token = current_token();
            error("Syntax error at line " + std::to_string(token.get_line()) +
                  ", col " + std::to_string(token.get_column()) +
                  ": declaration '" + Token::get_type_name(token.get_type()) +
                  "' appears out of order (expected order: const, type, var, subprogram)");
        } else {
            current_rank = rank;
        }

        const size_t before = pos;
        if (type == TokenType::TOKEN_CONSTSY) {
            append_declaration(parse_const_declaration(), "<const-declaration>", before);
        } else if (type == TokenType::TOKEN_TYPESY) {
            append_declaration(parse_type_declaration(), "<type-declaration>", before);
        } else if (type == TokenType::TOKEN_VARSY) {
            append_declaration(parse_var_declaration(), "<var-declaration>", before);
        } else {
            append_declaration(parse_subprogram_declaration(), "<subprogram-declaration>", before);
        }
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

    const size_t first_before = pos;
    node->add_child(parse_statement());
    if (pos == first_before && !is_at_end() && !check(TokenType::TOKEN_ENDSY) &&
        !check(TokenType::TOKEN_ELSESY) && !check(TokenType::TOKEN_UNTILSY)) {
        Token token = current_token();
        error("Syntax error at line " + std::to_string(token.get_line()) +
              ", col " + std::to_string(token.get_column()) +
              ": <statement> did not consume token " +
              Token::get_type_name(token.get_type()) +
              "; advancing to continue parsing");
        advance();
    }

    while (check(TokenType::TOKEN_SEMICOLON)) {
        node->add_child(terminal(advance()));
        const size_t before = pos;
        node->add_child(parse_statement());
        if (pos == before && !is_at_end() && !check(TokenType::TOKEN_ENDSY) &&
            !check(TokenType::TOKEN_ELSESY) && !check(TokenType::TOKEN_UNTILSY)) {
            Token token = current_token();
            error("Syntax error at line " + std::to_string(token.get_line()) +
                  ", col " + std::to_string(token.get_column()) +
                  ": <statement> did not consume token " +
                  Token::get_type_name(token.get_type()) +
                  "; advancing to continue parsing");
            advance();
        }
    }

    return node;
}
