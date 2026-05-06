#include "parser.hpp"
#include <string>


Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

std::shared_ptr<ParseTreeNode> Parser::parse() {
    return parse_program();
}

Token Parser::current_token() const {
    if (!tokens.empty() && pos < tokens.size()) return tokens[pos];
    if (!tokens.empty()) return tokens.back();
    return Token(TokenType::TOKEN_EOF, "", 0, 0);
}

Token Parser::peek(int offset) const {
    size_t idx = pos + static_cast<size_t>(offset);
    if (!tokens.empty() && idx < tokens.size()) return tokens[idx];
    if (!tokens.empty()) return tokens.back();
    return Token(TokenType::TOKEN_EOF, "", 0, 0);
}

bool Parser::is_at_end() const {
    return tokens.empty() ||
           pos >= tokens.size() ||
           current_token().get_type() == TokenType::TOKEN_EOF;
}

Token Parser::advance() {
    Token token = current_token();
    if (!is_at_end()) ++pos;
    return token;
}

bool Parser::check(TokenType type) const {
    return current_token().get_type() == type;
}

bool Parser::match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}

Token Parser::expect(TokenType type) {
    if (check(type)) return advance();

    Token token = current_token();
    error("Syntax error at line " + std::to_string(token.get_line()) +
          ", col " + std::to_string(token.get_column()) +
          ": unexpected token " + Token::get_type_name(token.get_type()) +
          ", expected " + Token::get_type_name(type));

    return token;
}



std::string Parser::terminal_name(const Token& token) const {
    std::string name = Token::get_type_name(token.get_type());
    TokenType type = token.get_type();

    if (type == TokenType::TOKEN_IDENT ||
        type == TokenType::TOKEN_INTCON ||
        type == TokenType::TOKEN_REALCON ||
        type == TokenType::TOKEN_CHARCON ||
        type == TokenType::TOKEN_STRING ||
        type == TokenType::TOKEN_ERROR) {
        return name + "(" + token.get_value() + ")";
    }

    return name;
}

std::shared_ptr<ParseTreeNode> Parser::terminal(Token token) {
    return make_node(terminal_name(token));
}

// error handling

void Parser::error(const std::string& msg) {
    errors.push_back(msg);
}

const std::vector<std::string>& Parser::get_errors() const {
    return errors;
}

void Parser::synchronize() {
    while (!is_at_end()) {
        if (check(TokenType::TOKEN_SEMICOLON) ||
            check(TokenType::TOKEN_ENDSY) ||
            check(TokenType::TOKEN_PERIOD)) {
            return;
        }
        advance();
    }
}
