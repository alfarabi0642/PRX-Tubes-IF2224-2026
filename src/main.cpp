#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "common/token.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/semantic.hpp"

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

    Lexer lexer(input_file);
    vector<Token> tokens = lexer.tokenize();

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

    vector<Token> filtered;
    for (const auto& token : tokens) {
        if (token.get_type() != TokenType::TOKEN_COMMENT &&
            token.get_type() != TokenType::TOKEN_NEWLINE) {
            filtered.push_back(token);
        }
    }

    Parser parser(filtered);
    auto tree = parser.parse();

    const auto& errors = parser.get_errors();

    tree->print(cout);

    string filename = input_file;
    size_t last_slash = filename.find_last_of("\\/");
    if (last_slash != string::npos) {
        filename = filename.substr(last_slash + 1);
    }
    string parse_output_path = "test/milestone-2/" + filename + "_OUTPUT.txt";

    ofstream parse_out(parse_output_path);
    if (parse_out.is_open()) {
        tree->print(parse_out);
        parse_out.close();
    } else {
        cerr << "Warning: Could not open output file: " << parse_output_path << endl;
    }

    if (!errors.empty()) {
        cerr << endl << "Parser errors (" << errors.size() << "):" << endl;
        for (const auto& err : errors) {
            cerr << "  " << err << endl;
        }
        return 1;
    }

    SemanticAnalyzer semantic;
    semantic.analyze(tree);
    semantic.print_report(cout);

    string semantic_output_path = "test/milestone-3/" + filename + "_SEMANTIC.txt";
    ofstream semantic_out(semantic_output_path);
    if (semantic_out.is_open()) {
        semantic_out << "=== Parse Tree ===" << endl;
        tree->print(semantic_out);
        semantic.print_report(semantic_out);
        semantic_out.close();
    } else {
        cerr << "Warning: Could not open output file: " << semantic_output_path << endl;
    }

    const auto& semantic_errors = semantic.get_errors();
    if (!semantic_errors.empty()) {
        cerr << endl << "Semantic errors (" << semantic_errors.size() << "):" << endl;
        for (const auto& err : semantic_errors) {
            cerr << "  " << err << endl;
        }
        return 1;
    }

    return 0;
}
