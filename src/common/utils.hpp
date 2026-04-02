#pragma once
#include <string>

/**
 * @class LexerUtils
 * @brief class helper
 */
class LexerUtils {
public:
    /**
     * @brief cek huruf
     */
    static bool is_alpha(char c);

    /**
     * @brief cek angka
     */
    static bool is_digit(char c);

    /**
     * @brief cek huruf dan angka
     */
    static bool is_alphanumeric(char c);

    /**
     * @brief mengubah string jadi huruf kecil
     */
    static std::string to_lowercase(const std::string& str);
};


