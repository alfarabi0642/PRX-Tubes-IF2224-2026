#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "../common/token.hpp"
#include "parse_tree.hpp"

class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;
    std::vector<std::string> errors;

    // parser_core.cpp
    Token current_token() const;
    Token peek(int offset = 0) const;
    bool is_at_end() const;
    Token advance();
    Token expect(TokenType type);
    bool check(TokenType type) const;
    bool match(TokenType type);

    std::string terminal_name(const Token& token) const;
    std::shared_ptr<ParseTreeNode> terminal(Token token);
    void error(const std::string& msg);
    void synchronize();

    // parser_toplevel.cpp
    std::shared_ptr<ParseTreeNode> parse_program();
    std::shared_ptr<ParseTreeNode> parse_program_header();
    std::shared_ptr<ParseTreeNode> parse_declaration_part();
    std::shared_ptr<ParseTreeNode> parse_block();
    std::shared_ptr<ParseTreeNode> parse_compound_statement();
    std::shared_ptr<ParseTreeNode> parse_statement_list();

    // parser_declarations.cpp
    std::shared_ptr<ParseTreeNode> parse_const_declaration();
    std::shared_ptr<ParseTreeNode> parse_constant();
    std::shared_ptr<ParseTreeNode> parse_type_declaration();
    std::shared_ptr<ParseTreeNode> parse_var_declaration();
    std::shared_ptr<ParseTreeNode> parse_identifier_list();
    std::shared_ptr<ParseTreeNode> parse_type();
    std::shared_ptr<ParseTreeNode> parse_array_type();
    std::shared_ptr<ParseTreeNode> parse_range();
    std::shared_ptr<ParseTreeNode> parse_enumerated();
    std::shared_ptr<ParseTreeNode> parse_record_type();
    std::shared_ptr<ParseTreeNode> parse_field_list();
    std::shared_ptr<ParseTreeNode> parse_field_part();

    // parser_statements.cpp
    std::shared_ptr<ParseTreeNode> parse_subprogram_declaration();
    std::shared_ptr<ParseTreeNode> parse_procedure_declaration();
    std::shared_ptr<ParseTreeNode> parse_function_declaration();
    std::shared_ptr<ParseTreeNode> parse_formal_parameter_list();
    std::shared_ptr<ParseTreeNode> parse_parameter_group();
    std::shared_ptr<ParseTreeNode> parse_statement();
    std::shared_ptr<ParseTreeNode> parse_variable();
    std::shared_ptr<ParseTreeNode> parse_component_variable();
    std::shared_ptr<ParseTreeNode> parse_index_list();
    std::shared_ptr<ParseTreeNode> parse_assignment_statement();
    std::shared_ptr<ParseTreeNode> parse_if_statement();

    // parser_expressions.cpp
    std::shared_ptr<ParseTreeNode> parse_case_statement();
    std::shared_ptr<ParseTreeNode> parse_case_block();
    std::shared_ptr<ParseTreeNode> parse_while_statement();
    std::shared_ptr<ParseTreeNode> parse_repeat_statement();
    std::shared_ptr<ParseTreeNode> parse_for_statement();
    std::shared_ptr<ParseTreeNode> parse_procedure_function_call();
    std::shared_ptr<ParseTreeNode> parse_parameter_list();
    std::shared_ptr<ParseTreeNode> parse_expression();
    std::shared_ptr<ParseTreeNode> parse_simple_expression();
    std::shared_ptr<ParseTreeNode> parse_term();
    std::shared_ptr<ParseTreeNode> parse_factor();
    std::shared_ptr<ParseTreeNode> parse_relational_operator();
    std::shared_ptr<ParseTreeNode> parse_additive_operator();
    std::shared_ptr<ParseTreeNode> parse_multiplicative_operator();

public:
    explicit Parser(const std::vector<Token>& tokens);
    std::shared_ptr<ParseTreeNode> parse();
    const std::vector<std::string>& get_errors() const;
};
