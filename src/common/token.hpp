#pragma once
#include <string>

/**
 * @enum TokenType
 * @brief enum semua token valid
 */
enum class TokenType {
    TOKEN_INTCON,       // 1.Integer
    TOKEN_REALCON,      // 2.Real
    TOKEN_CHARCON,      // 3. karakter 
    TOKEN_STRING,       // 4.String 
    TOKEN_NOTSY,        // 5. NOT
    TOKEN_PLUS,         // 6. + 
    TOKEN_MINUS,        // 7. - 
    TOKEN_TIMES,        // 8. * 
    TOKEN_IDIV,         // 9. div
    TOKEN_RDIV,         // 10. /
    TOKEN_IMOD,         // 11. MOD
    TOKEN_ANDSY,        // 12. AND
    TOKEN_ORSY,         // 13. OR
    TOKEN_EQL,          // 14. ==
    TOKEN_NEQ,          // 15. <>
    TOKEN_GTR,          // 16. >
    TOKEN_GEQ,          // 17. >=
    TOKEN_LSS,          // 18. <
    TOKEN_LEQ,          // 19. <=
    TOKEN_LPARENT,      // 20. (
    TOKEN_RPARENT,      // 21. )
    TOKEN_LBRACK,       // 22. [
    TOKEN_RBRACK,       // 23. ]
    TOKEN_COMMA,        // 24. ,
    TOKEN_SEMICOLON,    // 25. ;
    TOKEN_PERIOD,       // 26. .
    TOKEN_COLON,        // 27. :
    TOKEN_BECOMES,      // 28. :=
    TOKEN_CONSTSY,      // 29. const
    TOKEN_TYPESY,       // 30. type
    TOKEN_VARSY,        // 31. var
    TOKEN_FUNCTIONSY,   // 32. function
    TOKEN_PROCEDURESY,  // 33. procedure
    TOKEN_ARRAYSY,      // 34. array
    TOKEN_RECORDSY,     // 35. record
    TOKEN_PROGRAMSY,    // 36. program
    TOKEN_IDENT,        // 37. Identifier
    TOKEN_BEGINSY,      // 38 begin
    TOKEN_IFSY,         // 39 if
    TOKEN_CASESY,       // 40 case
    TOKEN_REPEATSY,     // 41 repeat
    TOKEN_WHILESY,      // 42 while
    TOKEN_FORSY,        // 43 for
    TOKEN_ENDSY,        // 44 end
    TOKEN_ELSESY,       // 45 else
    TOKEN_UNTILSY,      // 46 until
    TOKEN_OFSY,         // 47 of
    TOKEN_DOSY,         // 48 do
    TOKEN_TOSY,         // 49 to
    TOKEN_DOWNTOSY,     // 50 downto
    TOKEN_THENSY,       // 51 then
    TOKEN_COMMENT,      // 52 Komentar
    TOKEN_NEWLINE,      // Newline separator
    TOKEN_EOF,          // EOF
    TOKEN_ERROR         // UNKNOWN
};

/**
 * @class Token
 * @brief  nyimpen token yang sudah dipecah
 */
class Token {
private:
    TokenType type;
    std::string value;
    int line;
    int column;

public:
    /**
     * @brief konstruktor token
     */
    Token(TokenType type, std::string value, int line, int column) 
        : type(type), value(value), line(line), column(column) {}

    TokenType get_type() const { return type; }
    std::string get_value() const { return value; }
    int get_line() const { return line; }
    int get_column() const { return column; }

    /**
     * @brief ngubah token ke string output
     */
    static std::string get_type_name(TokenType type);
};

