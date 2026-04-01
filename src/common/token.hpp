#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

/**
 * @enum TokenType
 * @brief Represents all valid tokens within the Arion language specification.
 */
enum class TokenType {
    TOKEN_INTCON,       ///< 1. Konstanta integer
    TOKEN_REALCON,      ///< 2. Konstanta bilangan riil
    TOKEN_CHARCON,      ///< 3. Konstanta karakter singular
    TOKEN_STRING,       ///< 4. Sekuens karakter
    TOKEN_NOTSY,        ///< 5. Operator logika NOT
    TOKEN_PLUS,         ///< 6. Operator aritmatika pertambahan
    TOKEN_MINUS,        ///< 7. Operator aritmatika pengurangan
    TOKEN_TIMES,        ///< 8. Operator aritmatika perkalian
    TOKEN_IDIV,         ///< 9. Operator pembagian Integer (div)
    TOKEN_RDIV,         ///< 10. Operator pembagian Riil (/)
    TOKEN_IMOD,         ///< 11. Operator modulo (MOD)
    TOKEN_ANDSY,        ///< 12. Operator logika AND
    TOKEN_ORSY,         ///< 13. Operator logika OR
    TOKEN_EQL,          ///< 14. equal (==)
    TOKEN_NEQ,          ///< 15. Not equal (<>)
    TOKEN_GTR,          ///< 16. Greater than (>)
    TOKEN_GEQ,          ///< 17. Greater than or equal (>=)
    TOKEN_LSS,          ///< 18. Less than (<)
    TOKEN_LEQ,          ///< 19. Less than or equal (<=)
    TOKEN_LPARENT,      ///< 20. Left parentheses (()
    TOKEN_RPARENT,      ///< 21. Right parantheses ())
    TOKEN_LBRACK,       ///< 22. Kurung siku kiri ([)
    TOKEN_RBRACK,       ///< 23. Kurung siku kanan (])
    TOKEN_COMMA,        ///< 24. Comma (,)
    TOKEN_SEMICOLON,    ///< 25. Semicolon (;)
    TOKEN_PERIOD,       ///< 26. Period (.)
    TOKEN_COLON,        ///< 27. Colon (:)
    TOKEN_BECOMES,      ///< 28. Assignment operator (:=)
    TOKEN_CONSTSY,      ///< 29. Deklarasi konstanta (const)
    TOKEN_TYPESY,       ///< 30. Deklarasi tipe data (type)
    TOKEN_VARSY,        ///< 31. Deklarasi variabel (var)
    TOKEN_FUNCTIONSY,   ///< 32. Deklarasi fungsi (function)
    TOKEN_PROCEDURESY,  ///< 33. Deklarasi prosedur (procedure)
    TOKEN_ARRAYSY,      ///< 34. Deklarasi array (array)
    TOKEN_RECORDSY,     ///< 35. Deklarasi record (record)
    TOKEN_PROGRAMSY,    ///< 36. Deklarasi program (program)
    TOKEN_IDENT,        ///< 37. Identifier
    TOKEN_BEGINSY,      ///< 38. begin
    TOKEN_IFSY,         ///< 39. if
    TOKEN_CASESY,       ///< 40. case
    TOKEN_REPEATSY,     ///< 41. repeat
    TOKEN_WHILESY,      ///< 42. while
    TOKEN_FORSY,        ///< 43. for
    TOKEN_ENDSY,        ///< 44. end
    TOKEN_ELSESY,       ///< 45. else
    TOKEN_UNTILSY,      ///< 46. until
    TOKEN_OFSY,         ///< 47. of
    TOKEN_DOSY,         ///< 48. do
    TOKEN_TOSY,         ///< 49. to
    TOKEN_DOWNTOSY,     ///< 50. downto
    TOKEN_THENSY,       ///< 51. then
    TOKEN_COMMENT,      ///< 52. Komentar
    TOKEN_NEWLINE,      ///< Newline separator
    TOKEN_EOF,          ///< End of File
    TOKEN_ERROR         ///< Unrecognized token
};

/**
 * @class Token
 * @brief Stores parsed lexical tokens encapsulating type, string values, and positional data.
 */
class Token {
private:
    TokenType type;
    std::string value;
    int line;
    int column;

public:
    /**
     * @brief Constructs a new Token object.
     * @param type The enumerated token classification.
     * @param value The raw string literal matched in source.
     * @param line Line number where the token starts.
     * @param column Column number where the token starts.
     */
    Token(TokenType type, std::string value, int line, int column) 
        : type(type), value(value), line(line), column(column) {}

    TokenType get_type() const { return type; }
    std::string get_value() const { return value; }
    int get_line() const { return line; }
    int get_column() const { return column; }

    /**
     * @brief Resolves a token classification enum into its string representation required by spec.
     * @param type The Token Type Enum.
     * @return std::string Spec corresponding string (e.g. "intcon", "ident").
     */
    static std::string get_type_name(TokenType type);
};

#endif // TOKEN_HPP
