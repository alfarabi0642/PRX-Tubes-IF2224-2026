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
    S_UNKNOWN, // State untuk akumulasi unknown token sampai separator
    S99
};

/**
 * @class Lexer
 * @brief nge parse input pake DFA jadi language
 */
class Lexer {
private:
    FILE* source_file;       // file pointer ke input (source code)
    int line;                // curr line
    int column;              // curr column
    std::stack<int> unget_buffer; // buffer buat unread char
    bool next_number_negative;    // flag buat edge case integer negatif
    bool second_range_period_pending; // allow the second dot in ranges like 1..10
    
    /**
     * @brief nge parse sequence jadi token pake DFA
     */
    Token get_next_token();

public:
    /**
     * @brief constructor lexer
     */
    Lexer(const std::string& filename);

    /**
     * @brief Destructor lexer
     */
    ~Lexer();

    /**
     * @brief main funk
     */
    std::vector<Token> tokenize();
};

