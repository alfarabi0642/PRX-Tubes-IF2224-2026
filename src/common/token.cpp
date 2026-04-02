#include "token.hpp"

std::string Token::get_type_name(TokenType type) {
    switch(type) {
        case TokenType::TOKEN_INTCON: return "intcon";
        case TokenType::TOKEN_REALCON: return "realcon";
        case TokenType::TOKEN_CHARCON: return "charcon";
        case TokenType::TOKEN_STRING: return "string";
        case TokenType::TOKEN_NOTSY: return "notsy";
        case TokenType::TOKEN_PLUS: return "plus";
        case TokenType::TOKEN_MINUS: return "minus";
        case TokenType::TOKEN_TIMES: return "times";
        case TokenType::TOKEN_IDIV: return "idiv";
        case TokenType::TOKEN_RDIV: return "rdiv";
        case TokenType::TOKEN_IMOD: return "imod";
        case TokenType::TOKEN_ANDSY: return "andsy";
        case TokenType::TOKEN_ORSY: return "orsy";
        case TokenType::TOKEN_EQL: return "eql";
        case TokenType::TOKEN_NEQ: return "neq";
        case TokenType::TOKEN_GTR: return "gtr";
        case TokenType::TOKEN_GEQ: return "geq";
        case TokenType::TOKEN_LSS: return "lss";
        case TokenType::TOKEN_LEQ: return "leq";
        case TokenType::TOKEN_LPARENT: return "lparent";
        case TokenType::TOKEN_RPARENT: return "rparent";
        case TokenType::TOKEN_LBRACK: return "lbrack";
        case TokenType::TOKEN_RBRACK: return "rbrack";
        case TokenType::TOKEN_COMMA: return "comma";
        case TokenType::TOKEN_SEMICOLON: return "semicolon";
        case TokenType::TOKEN_PERIOD: return "period";
        case TokenType::TOKEN_COLON: return "colon";
        case TokenType::TOKEN_BECOMES: return "becomes";
        case TokenType::TOKEN_CONSTSY: return "constsy";
        case TokenType::TOKEN_TYPESY: return "typesy";
        case TokenType::TOKEN_VARSY: return "varsy";
        case TokenType::TOKEN_FUNCTIONSY: return "functionsy";
        case TokenType::TOKEN_PROCEDURESY: return "proceduresy";
        case TokenType::TOKEN_ARRAYSY: return "arraysy";
        case TokenType::TOKEN_RECORDSY: return "recordsy";
        case TokenType::TOKEN_PROGRAMSY: return "programsy";
        case TokenType::TOKEN_IDENT: return "ident";
        case TokenType::TOKEN_BEGINSY: return "beginsy";
        case TokenType::TOKEN_IFSY: return "ifsy";
        case TokenType::TOKEN_CASESY: return "casesy";
        case TokenType::TOKEN_REPEATSY: return "repeatsy";
        case TokenType::TOKEN_WHILESY: return "whilesy";
        case TokenType::TOKEN_FORSY: return "forsy";
        case TokenType::TOKEN_ENDSY: return "endsy";
        case TokenType::TOKEN_ELSESY: return "elsesy";
        case TokenType::TOKEN_UNTILSY: return "untilsy";
        case TokenType::TOKEN_OFSY: return "ofsy";
        case TokenType::TOKEN_DOSY: return "dosy";
        case TokenType::TOKEN_TOSY: return "tosy";
        case TokenType::TOKEN_DOWNTOSY: return "downtosy";
        case TokenType::TOKEN_THENSY: return "thensy";
        case TokenType::TOKEN_COMMENT: return "comment";
        case TokenType::TOKEN_NEWLINE: return "newline";
        case TokenType::TOKEN_EOF: return "eof";
        default: return "unknown";
    }
}
