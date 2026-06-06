#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <stack>
#include "../common/token.hpp"

enum class State {
    S0, S1, S2, S3, S4, S5, S6, S7, 
    S10, S11, S12, S13, S14,
    S20, S21, S22, S23, S24,
    S30, S31, S32,
    S40, S41, S42, S43, S44, S45, S46, S47, S48, S49, S50,
    S60, S61, S62, S63, S64, S65, S66, S67, S68, S69, S70, S71,
    S_UNKNOWN, // Accumulate unknown token
    S99
};

/**
 * @class Lexer
 * @brief Tokenizes input with DFA
 */
class Lexer {
private:
    FILE* source_file;       // Source file pointer
    int line;                // Current line
    int column;              // Current column
    std::stack<int> unget_buffer; // Unread char buffer
    bool next_number_negative;    // Negative number flag
    bool second_range_period_pending; // Range dot flag
    
    /**
     * @brief Reads next token
     */
    Token get_next_token();

public:
    /**
     * @brief Lexer constructor
     */
    Lexer(const std::string& filename);

    /**
     * @brief Lexer destructor
     */
    ~Lexer();

    /**
     * @brief Tokenize source
     */
    std::vector<Token> tokenize();
};

