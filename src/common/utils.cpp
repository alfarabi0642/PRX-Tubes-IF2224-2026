#include "utils.hpp"
#include <algorithm>
#include <cctype>

bool LexerUtils::is_alpha(char c) {
    // TODO: Return true if 'c' is A-Z or a-z
    return std::isalpha(static_cast<unsigned char>(c)); // Placeholder example
}

bool LexerUtils::is_digit(char c) {
    // TODO: Return true if 'c' is 0-9
    return std::isdigit(static_cast<unsigned char>(c)); // Placeholder example
}

bool LexerUtils::is_alphanumeric(char c) {
    // TODO: Return true if 'c' is alphanumeric
    return std::isalnum(static_cast<unsigned char>(c)); // Placeholder example
}

std::string LexerUtils::to_lowercase(const std::string& str) {
    // TODO: Implement case conversion to help handling case-insensitive keywords and identifiers
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lower_str;
}
