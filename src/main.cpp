#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "common/token.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

using namespace std;

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

    // Step 1: Run lexer
    Lexer lexer(input_file);
    vector<Token> tokens = lexer.tokenize();

    // Step 2: Check for lexer errors — report and stop
    bool has_lexer_error = false;
    for (const auto& token : tokens) {
        if (token.get_type() == TokenType::TOKEN_ERROR) {
            has_lexer_error = true;
            cerr << "Lexical error at line " << token.get_line()
                 << ", col " << token.get_column()
                 << ": " << token.get_value() << endl;
        }
    }

    if (has_lexer_error) {
        cerr << "Parsing aborted due to lexical errors." << endl;
        return 1;
    }

    // Step 3: Filter TOKEN_COMMENT and TOKEN_NEWLINE
    vector<Token> filtered;
    for (const auto& token : tokens) {
        if (token.get_type() != TokenType::TOKEN_COMMENT &&
            token.get_type() != TokenType::TOKEN_NEWLINE) {
            filtered.push_back(token);
        }
    }

    // Step 4: Parse
    Parser parser(filtered);
    auto tree = parser.parse();

    // Step 5: Print parse tree to stdout
    tree->print(cout);

    // Step 6: Save parse tree to output file
    string filename = input_file;
    size_t last_slash = filename.find_last_of("\\/");
    if (last_slash != string::npos) {
        filename = filename.substr(last_slash + 1);
    }
    string output_path = "test/milestone-2/" + filename + "_OUTPUT.txt";

    ofstream out(output_path);
    if (out.is_open()) {
        tree->print(out);
        out.close();
    } else {
        cerr << "Warning: Could not open output file: " << output_path << endl;
    }

    // Step 7: Print parser errors
    const auto& errors = parser.get_errors();
    if (!errors.empty()) {
        cerr << endl << "Parser errors (" << errors.size() << "):" << endl;
        for (const auto& err : errors) {
            cerr << "  " << err << endl;
        }
        return 1;
    }

    return 0;
}
