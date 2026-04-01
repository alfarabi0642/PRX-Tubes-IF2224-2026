#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

/**
 * @class LexerUtils
 * @brief Provides stateless static helper utilities for evaluating DFA transition conditions such as 
 *        character types and case-insensitivity formatting.
 */
class LexerUtils {
public:
    /**
     * @brief Checks if a character is an alphabetic letter [a-zA-Z].
     * @param c Character to test.
     * @return true if alphabetic.
     */
    static bool is_alpha(char c);

    /**
     * @brief Checks if a character is a numeric digit [0-9].
     * @param c Character to test.
     * @return true if numeric.
     */
    static bool is_digit(char c);

    /**
     * @brief Checks if a character is alphanumeric [a-zA-Z0-9].
     * @param c Character to test.
     * @return true if alphanumeric.
     */
    static bool is_alphanumeric(char c);

    /**
     * @brief Converts a given string fully to lowercase.
     *        Used for case-insensitive matching of keywords and checking.
     * @param str The input string.
     * @return lowercase mapped string.
     */
    static std::string to_lowercase(const std::string& str);
};

#endif // UTILS_HPP
