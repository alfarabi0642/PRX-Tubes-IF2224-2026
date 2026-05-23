#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "common/token.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/ast_builder.hpp"
#include "semantic/diagnostic.hpp"
#include "semantic/printer.hpp"
#include "semantic/semantic_analyzer.hpp"

using namespace std;

namespace {

// Append diagnostic list
void append_diagnostics(vector<semantic::Diagnostic>& target,
                        const vector<semantic::Diagnostic>& source) {
    target.insert(target.end(), source.begin(), source.end());
}

// Print error summary
void print_diagnostic_summary(const vector<semantic::Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity != semantic::DiagnosticSeverity::Error) continue;
        cerr << "  ";
        if (diagnostic.location.line > 0 || diagnostic.location.column > 0) {
            cerr << diagnostic.location.line << ':' << diagnostic.location.column << ": ";
        }
        cerr << diagnostic.message << endl;
    }
}

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

    // Stop lexical errors
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

    // Filter parser noise
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

    cout << "=== Parse Tree ===" << endl;
    tree->print(cout);

    string filename = input_file;
    size_t last_slash = filename.find_last_of("\\/");
    if (last_slash != string::npos) {
        filename = filename.substr(last_slash + 1);
    }
    if (!errors.empty()) {
        cerr << endl << "Parser errors (" << errors.size() << "):" << endl;
        for (const auto& err : errors) {
            cerr << "  " << err << endl;
        }
        return 1;
    }

    // Build compact AST
    semantic::AstBuilder ast_builder;
    auto ast_result = ast_builder.build(tree);

    semantic::SemanticResult semantic_result;
    vector<semantic::Diagnostic> diagnostics;
    append_diagnostics(diagnostics, ast_result.diagnostics);

    if (ast_result.root) {
        // Decorate semantic AST
        semantic::SemanticAnalyzer analyzer;
        semantic_result = analyzer.analyze(ast_result.root);
        append_diagnostics(diagnostics, semantic_result.diagnostics);
    }

    // Print semantic report
    const auto report_root = semantic_result.decorated_ast ? semantic_result.decorated_ast : ast_result.root;
    semantic::print_semantic_report(cout, report_root,
                                    semantic_result.symbols,
                                    semantic_result.types,
                                    diagnostics);

    // Save semantic report
    string semantic_output_path = "test/milestone-3/" + filename + "_SEMANTIC.txt";
    ofstream semantic_out(semantic_output_path);
    if (semantic_out.is_open()) {
        semantic_out << "=== Parse Tree ===" << endl;
        tree->print(semantic_out);
        semantic::print_semantic_report(semantic_out, report_root,
                                        semantic_result.symbols,
                                        semantic_result.types,
                                        diagnostics);
        semantic_out.close();
    } else {
        cerr << "Warning: Could not open output file: " << semantic_output_path << endl;
    }

    if (semantic::has_errors(diagnostics)) {
        cerr << endl << "Semantic errors:" << endl;
        print_diagnostic_summary(diagnostics);
        return 1;
    }

    return 0;
}
