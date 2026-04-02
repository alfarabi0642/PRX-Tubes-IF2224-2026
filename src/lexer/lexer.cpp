#include "lexer.hpp"
#include "../common/utils.hpp"
#include <iostream>

Lexer::Lexer(const std::string& filename) : line(1), column(1), next_number_negative(false) {
    source_file = fopen(filename.c_str(), "r");
    if (!source_file) {
        std::cerr << "Lexical Error: Cannot open source file " << filename << std::endl;
    }
}

Lexer::~Lexer() {
    if (source_file) {
        fclose(source_file);
    }
}

Token Lexer::get_next_token() {
    if (!source_file) {
        return Token(TokenType::TOKEN_EOF, "", line, column);
    }

    State currentState = State::S0;
    std::string currentLexeme = "";
    int start_line = line;
    int start_col = column;

    auto read_char = [&]() -> int {
        int c;
        if (!unget_buffer.empty()) {
            c = unget_buffer.top();
            unget_buffer.pop();
        } else {
            c = fgetc(source_file);
        }

        if (c != EOF) {
            if (c == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
        }
        return c;
    };

    auto unread_char = [&](int c, int prev_l, int prev_col) {
        if (c != EOF) {
            unget_buffer.push(c);
            line = prev_l;
            column = prev_col;
        }
    };

    while (true) {
        int prev_line = line;
        int prev_col = column;
        int c = read_char();

        switch (currentState) {
            case State::S0:
                start_line = prev_line;
                start_col = prev_col;
                currentLexeme = "";
                
                if (c == EOF) {
                    currentState = State::S99;
                    break;
                }

                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    currentState = State::S2;
                } else if (c == '{') {
                    currentState = State::S3;
                    currentLexeme += (char)c;
                } else if (c == '(') {
                    currentState = State::S4;
                    currentLexeme += (char)c;
                } else if (LexerUtils::is_digit(c)) {
                    currentState = State::S10;
                    if (this->next_number_negative) {
                        currentLexeme += '-';
                        this->next_number_negative = false;
                    }
                    currentLexeme += (char)c;
                } else if (c == '\'') {
                    currentState = State::S20;
                    currentLexeme += (char)c;
                } else if (LexerUtils::is_alpha(c)) {
                    // QNA 30: '_' is invalid as the FIRST character of identifier.
                    // The DFA says S0 -> S30 [a-zA-Z]
                    currentState = State::S30;
                    currentLexeme += (char)c;
                } else if (c == '+') { currentState = State::S40; currentLexeme += (char)c; }
                  else if (c == '-') { currentState = State::S41; currentLexeme += (char)c; }
                  else if (c == '*') { currentState = State::S42; currentLexeme += (char)c; }
                  else if (c == '/') { currentState = State::S43; currentLexeme += (char)c; }
                  else if (c == ')') { currentState = State::S45; currentLexeme += (char)c; }
                  else if (c == '[') { currentState = State::S46; currentLexeme += (char)c; }
                  else if (c == ']') { currentState = State::S47; currentLexeme += (char)c; }
                  else if (c == ',') { currentState = State::S48; currentLexeme += (char)c; }
                  else if (c == ';') { currentState = State::S49; currentLexeme += (char)c; }
                  else if (c == '.') { currentState = State::S50; currentLexeme += (char)c; }
                  else if (c == ':') { currentState = State::S60; currentLexeme += (char)c; }
                  else if (c == '=') { currentState = State::S63; currentLexeme += (char)c; }
                  else if (c == '<') { currentState = State::S65; currentLexeme += (char)c; }
                  else if (c == '>') { currentState = State::S69; currentLexeme += (char)c; }
                  else {
                      currentState = State::S1;
                      currentLexeme += (char)c;
                  }
                break;

            case State::S1:
                std::cerr << "Lexical Error: Unrecognized or invalid sequence '" << currentLexeme 
                          << "' at line " << start_line << ", col " << start_col << std::endl;
                
                // Consumes offending character but returns as ERROR token.
                // Resetting to S0 will naturally happen on the next get_next_token call.
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_ERROR, currentLexeme, start_line, start_col);

            case State::S2:
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    currentState = State::S2;
                } else {
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S0; // Loop back without emitting
                }
                break;

            case State::S3:
            case State::S5:
                if (c == EOF) {
                    currentState = State::S1; // EOF (Unclosed) - reaches error
                } else if (c == '}') {
                    currentState = State::S7;
                    currentLexeme += (char)c;
                } else if (c == '*') {
                    currentState = State::S6;
                    currentLexeme += (char)c;
                } else {
                    currentState = State::S5;
                    currentLexeme += (char)c;
                }
                break;

            case State::S4:
                if (c == '*') {
                    currentState = State::S5;
                    currentLexeme += (char)c;
                } else {
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S44; // LPARENT_ACCEPT
                }
                break;

            case State::S6:
                if (c == EOF) {
                    currentState = State::S1;
                } else if (c == ')') {
                    currentState = State::S7;
                    currentLexeme += (char)c;
                } else if (c == '}') {
                    currentState = State::S7;
                    currentLexeme += (char)c;
                } else if (c == '*') {
                    currentState = State::S6;
                    currentLexeme += (char)c;
                } else {
                    currentState = State::S5;
                    currentLexeme += (char)c;
                }
                break;

            case State::S7:
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_COMMENT, currentLexeme, start_line, start_col);

            case State::S10:
                if (LexerUtils::is_digit(c)) {
                    currentState = State::S10;
                    currentLexeme += (char)c;
                } else if (c == '.') {
                    currentState = State::S12;
                    currentLexeme += (char)c;
                } else { // bukan [0-9.]
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S11;
                }
                break;

            case State::S11:
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_INTCON, currentLexeme, start_line, start_col);

            case State::S12:
                if (LexerUtils::is_digit(c)) {
                    currentState = State::S13;
                    currentLexeme += (char)c;
                } else {
                    // bukan [0-9] (unget '.', emit intcon)
                    unread_char(c, prev_line, prev_col);
                    unread_char('.', start_line, start_col + currentLexeme.length() - 1);
                    currentLexeme.pop_back(); // Remove '.' from lexeme string
                    currentState = State::S11;
                }
                break;

            case State::S13:
                if (LexerUtils::is_digit(c)) {
                    currentState = State::S13;
                    currentLexeme += (char)c;
                } else if (c == 'e') { // 'e' -> S1 (QNA 18)
                    currentState = State::S1;
                    currentLexeme += (char)c;
                } else { // bukan [0-9]
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S14;
                }
                break;

            case State::S14:
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_REALCON, currentLexeme, start_line, start_col);

            case State::S20:
                // Note: EOF check is done implicitly if c == EOF, S21 will eventually handle it for unclosed string.
                if (c == '\'') {
                    currentState = State::S22;
                    currentLexeme += (char)c;
                } else if (c == '\n' || c == EOF) { // Error or EOF
                    currentState = State::S1;
                    if (c != EOF) currentLexeme += (char)c;
                } else {
                    currentState = State::S21;
                    currentLexeme += (char)c;
                }
                break;

            case State::S21:
                if (c == '\n' || c == EOF) { // \n or EOF (QNA 14 & 22)
                    currentState = State::S1;
                    if (c != EOF) currentLexeme += (char)c;
                } else if (c == '\'') {
                    currentState = State::S22;
                    currentLexeme += (char)c;
                } else {
                    currentState = State::S21;
                    currentLexeme += (char)c;
                }
                break;

            case State::S22:
                if (c == '\'') { // Escaped single quote
                    currentState = State::S21;
                    currentLexeme += (char)c;
                } else {
                    unread_char(c, prev_line, prev_col);
                    
                    int actual_len = 0;
                    for (size_t i = 1; i < currentLexeme.length() - 1; i++) {
                        if (currentLexeme[i] == '\'') {
                            if (i + 1 < currentLexeme.length() - 1 && currentLexeme[i+1] == '\'') {
                                i++; // skip escaped quote part
                            }
                        }
                        actual_len++;
                    }

                    if (actual_len == 1) {
                        currentState = State::S23;
                    } else {
                        currentState = State::S24;
                    }
                }
                break;

            case State::S23:
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_CHARCON, currentLexeme, start_line, start_col);

            case State::S24:
                unread_char(c, prev_line, prev_col);
                return Token(TokenType::TOKEN_STRING, currentLexeme, start_line, start_col);

            case State::S30:
                if (LexerUtils::is_alphanumeric(c)) { // [a-z,A-Z0-9] (QNA 30: '_' is invalid)
                    currentState = State::S31;
                    currentLexeme += (char)c;
                } else {
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S32;
                }
                break;

            case State::S31:
                if (LexerUtils::is_alphanumeric(c)) {
                    currentState = State::S31;
                    currentLexeme += (char)c;
                } else {
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S32;
                }
                break;

            case State::S32: {
                unread_char(c, prev_line, prev_col);
                std::string lower_ident = LexerUtils::to_lowercase(currentLexeme);
                if (lower_ident == "not") return Token(TokenType::TOKEN_NOTSY, currentLexeme, start_line, start_col);
                if (lower_ident == "div") return Token(TokenType::TOKEN_IDIV, currentLexeme, start_line, start_col);
                if (lower_ident == "mod") return Token(TokenType::TOKEN_IMOD, currentLexeme, start_line, start_col);
                if (lower_ident == "and") return Token(TokenType::TOKEN_ANDSY, currentLexeme, start_line, start_col);
                if (lower_ident == "or") return Token(TokenType::TOKEN_ORSY, currentLexeme, start_line, start_col);
                if (lower_ident == "const") return Token(TokenType::TOKEN_CONSTSY, currentLexeme, start_line, start_col);
                if (lower_ident == "type") return Token(TokenType::TOKEN_TYPESY, currentLexeme, start_line, start_col);
                if (lower_ident == "var") return Token(TokenType::TOKEN_VARSY, currentLexeme, start_line, start_col);
                if (lower_ident == "function") return Token(TokenType::TOKEN_FUNCTIONSY, currentLexeme, start_line, start_col);
                if (lower_ident == "procedure") return Token(TokenType::TOKEN_PROCEDURESY, currentLexeme, start_line, start_col);
                if (lower_ident == "array") return Token(TokenType::TOKEN_ARRAYSY, currentLexeme, start_line, start_col);
                if (lower_ident == "record") return Token(TokenType::TOKEN_RECORDSY, currentLexeme, start_line, start_col);
                if (lower_ident == "program") return Token(TokenType::TOKEN_PROGRAMSY, currentLexeme, start_line, start_col);
                if (lower_ident == "begin") return Token(TokenType::TOKEN_BEGINSY, currentLexeme, start_line, start_col);
                if (lower_ident == "if") return Token(TokenType::TOKEN_IFSY, currentLexeme, start_line, start_col);
                if (lower_ident == "case") return Token(TokenType::TOKEN_CASESY, currentLexeme, start_line, start_col);
                if (lower_ident == "repeat") return Token(TokenType::TOKEN_REPEATSY, currentLexeme, start_line, start_col);
                if (lower_ident == "while") return Token(TokenType::TOKEN_WHILESY, currentLexeme, start_line, start_col);
                if (lower_ident == "for") return Token(TokenType::TOKEN_FORSY, currentLexeme, start_line, start_col);
                if (lower_ident == "end") return Token(TokenType::TOKEN_ENDSY, currentLexeme, start_line, start_col);
                if (lower_ident == "else") return Token(TokenType::TOKEN_ELSESY, currentLexeme, start_line, start_col);
                if (lower_ident == "until") return Token(TokenType::TOKEN_UNTILSY, currentLexeme, start_line, start_col);
                if (lower_ident == "of") return Token(TokenType::TOKEN_OFSY, currentLexeme, start_line, start_col);
                if (lower_ident == "do") return Token(TokenType::TOKEN_DOSY, currentLexeme, start_line, start_col);
                if (lower_ident == "to") return Token(TokenType::TOKEN_TOSY, currentLexeme, start_line, start_col);
                if (lower_ident == "downto") return Token(TokenType::TOKEN_DOWNTOSY, currentLexeme, start_line, start_col);
                if (lower_ident == "then") return Token(TokenType::TOKEN_THENSY, currentLexeme, start_line, start_col);
                return Token(TokenType::TOKEN_IDENT, currentLexeme, start_line, start_col);
            }

            case State::S40: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_PLUS, currentLexeme, start_line, start_col);
            case State::S41: 
                unread_char(c, prev_line, prev_col); 
                // QNA 42: Record negative sign into next integer payload if next character is digit
                if (LexerUtils::is_digit(c)) {
                    this->next_number_negative = true;
                }
                return Token(TokenType::TOKEN_MINUS, currentLexeme, start_line, start_col);
            case State::S42: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_TIMES, currentLexeme, start_line, start_col);
            case State::S43: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_RDIV, currentLexeme, start_line, start_col);
            case State::S44: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_LPARENT, currentLexeme, start_line, start_col);
            case State::S45: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_RPARENT, currentLexeme, start_line, start_col);
            case State::S46: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_LBRACK, currentLexeme, start_line, start_col);
            case State::S47: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_RBRACK, currentLexeme, start_line, start_col);
            case State::S48: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_COMMA, currentLexeme, start_line, start_col);
            case State::S49: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_SEMICOLON, currentLexeme, start_line, start_col);
            case State::S50: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_PERIOD, currentLexeme, start_line, start_col);

            case State::S60:
                if (c == '=') {
                    currentState = State::S62;
                    currentLexeme += (char)c;
                } else {
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S61;
                }
                break;
            
            case State::S61: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_COLON, currentLexeme, start_line, start_col);
            case State::S62: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_BECOMES, currentLexeme, start_line, start_col);

            case State::S63: // EQL_INTER
                if (c == '=') {
                    currentState = State::S64;
                    currentLexeme += (char)c;
                } else { // bukan '=' -> S1 (ERROR)
                    unread_char(c, prev_line, prev_col);
                    currentState = State::S1;
                }
                break;

            case State::S64: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_EQL, currentLexeme, start_line, start_col);

            case State::S65:
                if (c == '=') { currentState = State::S67; currentLexeme += (char)c; }
                else if (c == '>') { currentState = State::S68; currentLexeme += (char)c; }
                else { unread_char(c, prev_line, prev_col); currentState = State::S66; }
                break;

            case State::S66: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_LSS, currentLexeme, start_line, start_col);
            case State::S67: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_LEQ, currentLexeme, start_line, start_col);
            case State::S68: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_NEQ, currentLexeme, start_line, start_col);

            case State::S69:
                if (c == '=') { currentState = State::S71; currentLexeme += (char)c; }
                else { unread_char(c, prev_line, prev_col); currentState = State::S70; }
                break;

            case State::S70: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_GTR, currentLexeme, start_line, start_col);
            case State::S71: unread_char(c, prev_line, prev_col); return Token(TokenType::TOKEN_GEQ, currentLexeme, start_line, start_col);

            case State::S99:
                unread_char(c, prev_line, prev_col); // put EOF back or drop doesn't matter much natively
                return Token(TokenType::TOKEN_EOF, "", start_line, start_col);
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        Token t = get_next_token();
        if (t.get_type() == TokenType::TOKEN_EOF) {
            tokens.push_back(t);
            break;
        }
        tokens.push_back(t);
    }
    return tokens;
}
