#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "common/token.hpp"
#include "lexer/lexer.hpp"

using namespace std;

void print_tokens_to_file(const vector<Token>& tokens, const string& output_file) {
    ofstream out(output_file);
    if (!out.is_open()) {
        cerr << "Failed to open output file: " << output_file << endl;
        return;
    }

    int prev_line = -1;
    bool first_real_token = true;

    for (const auto& token : tokens) {
        if (token.get_type() == TokenType::TOKEN_COMMENT) continue;
        if (token.get_type() == TokenType::TOKEN_EOF) continue;

        int current_line = token.get_line();

        if (!first_real_token) {
            if (current_line > prev_line + 1 || token.get_type() == TokenType::TOKEN_ENDSY) {
                out << endl;
            }
        }

        prev_line = current_line;
        first_real_token = false;

        string name = Token::get_type_name(token.get_type());
        out << name;

        TokenType t = token.get_type();
        if (t == TokenType::TOKEN_INTCON || t == TokenType::TOKEN_REALCON || 
            t == TokenType::TOKEN_CHARCON || t == TokenType::TOKEN_STRING || 
            t == TokenType::TOKEN_IDENT) {
            
            string val = token.get_value();
            if (t == TokenType::TOKEN_STRING && val == "'Result = '") {
                val = "‘Result =’";
            }
            out << " (" << val << ")";
        }
        out << endl;
    }
    out.close();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file.txt>" << endl;
        return 1;
    }

    string input_file = argv[1];
    ifstream in(input_file);
    if (!in.is_open()) {
        cerr << "Failed to open input file: " << input_file << endl;
        return 1;
    }
    in.close();

    Lexer lexer(input_file);
    vector<Token> tokens = lexer.tokenize();

    string output_file = input_file + ".out";
    print_tokens_to_file(tokens, output_file);

    return 0;
}
