#include "utils.hpp"
#include <algorithm>
#include <cctype>

bool LexerUtils::is_alpha(char c) {
    return std::isalpha(static_cast<unsigned char>(c)); 
}

bool LexerUtils::is_digit(char c) {
    return std::isdigit(static_cast<unsigned char>(c)); 
}

bool LexerUtils::is_alphanumeric(char c) {
    return std::isalnum(static_cast<unsigned char>(c)); 
}

std::string LexerUtils::to_lowercase(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), [](unsigned char c){
        return std::tolower(c); 
    });
    return lower_str;
}
