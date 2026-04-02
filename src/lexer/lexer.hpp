#ifndef LEXER_HPP
#define LEXER_HPP

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
    S99
};

/**
 * @class Lexer
 * @brief Parses source code applying a Deterministic Finite Automaton (DFA) string matching scheme 
 *        to produce language-specification tokens.
 */
class Lexer {
private:
    FILE* source_file;       ///< Source code file pointer
    int line;                ///< Current line number (1-indexed)
    int column;              ///< Current column number (1-indexed)
    std::stack<int> unget_buffer; ///< Buffer for safe multiple unread_char pushbacks
    bool next_number_negative;    ///< Flag for Q42 negative integer edge case tracking
    
    /**
     * @brief Processes the current sequence to build the next valid token using 
     *        a strict finite automata logic, reading precisely char by char.
     * @return Token Data Token object generated.
     */
    Token get_next_token();

public:
    /**
     * @brief Starts the lexical analyzer mapping engine.
     * @param filename The path to the source file.
     */
    Lexer(const std::string& filename);

    /**
     * @brief Destructor to close the file pointer.
     */
    ~Lexer();

    /**
     * @brief Exhaustively evaluates all tokens linearly until EOF.
     * @return vector<Token> list of sequentially ordered evaluation Tokens.
     */
    std::vector<Token> tokenize();
};

#endif // LEXER_HPP
