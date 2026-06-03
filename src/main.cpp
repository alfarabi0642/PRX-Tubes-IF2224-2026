#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include "common/token.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "backend/code_generator.hpp"
#include "backend/interpreter.hpp"
#include "backend/tac.hpp"
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

void print_text_diagnostics(ostream& os, const vector<string>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        os << diagnostic << endl;
    }
}

void print_intermediate_code(ostream& os, const vector<backend::Instruction>& instructions) {
    os << endl << "=== Intermediate Code ===" << endl;
    for (size_t i = 0; i < instructions.size(); ++i) {
        os << backend::format_instruction(i, instructions[i]) << endl;
    }
}

void print_program_output(ostream& os, const string& output) {
    os << endl << "=== Program Output ===" << endl;
    os << output;
    if (!output.empty() && output.back() != '\n') {
        os << endl;
    }
}

void print_diagnostic_section(ostream& os,
                              const string& title,
                              const vector<string>& diagnostics) {
    os << endl << "=== " << title << " ===" << endl;
    print_text_diagnostics(os, diagnostics);
}

string normalize_path(string path) {
    for (char& ch : path) {
        if (ch == '\\') {
            ch = '/';
        } else {
            ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        }
    }
    return path;
}

string parent_directory(const string& path) {
    const size_t last_slash = path.find_last_of("\\/");
    if (last_slash == string::npos) {
        return ".";
    }
    return path.substr(0, last_slash);
}

string join_path(const string& directory, const string& filename) {
    if (directory.empty() || directory == ".") {
        return filename;
    }
    const char last = directory.back();
    if (last == '\\' || last == '/') {
        return directory + filename;
    }
    return directory + "\\" + filename;
}

enum class InputLocation {
    Milestone3,
    Milestone4,
    Other
};

InputLocation classify_input_location(const string& input_file) {
    const string normalized = normalize_path(input_file);
    if (normalized.find("test/milestone-4/") != string::npos) {
        return InputLocation::Milestone4;
    }
    if (normalized.find("test/milestone-3/") != string::npos) {
        return InputLocation::Milestone3;
    }
    return InputLocation::Other;
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

    const InputLocation input_location = classify_input_location(input_file);
    const string input_directory = parent_directory(input_file);
    const string semantic_output_path = join_path(input_directory, filename + "_SEMANTIC.txt");
    const string backend_output_path = join_path(input_directory, filename + "_BACKEND.txt");

    auto write_report_file = [&](const string& output_path, auto write_extra_sections) {
        ofstream out(output_path);
        if (!out.is_open()) {
            cerr << "Warning: Could not open output file: " << output_path << endl;
            return;
        }

        out << "=== Parse Tree ===" << endl;
        tree->print(out);
        semantic::print_semantic_report(out, report_root,
                                        semantic_result.symbols,
                                        semantic_result.types,
                                        diagnostics);
        write_extra_sections(out);
    };

    if (input_location != InputLocation::Milestone4) {
        write_report_file(semantic_output_path, [](ostream&) {});
    }

    if (semantic::has_errors(diagnostics)) {
        if (input_location == InputLocation::Milestone4) {
            write_report_file(backend_output_path, [](ostream&) {});
        }
        cerr << endl << "Semantic errors:" << endl;
        print_diagnostic_summary(diagnostics);
        return 1;
    }

    if (!semantic_result.decorated_ast) {
        const vector<string> backend_diagnostics = {
            "Code generation skipped: decorated AST is missing."
        };
        if (input_location != InputLocation::Milestone3) {
            write_report_file(backend_output_path, [&](ostream& out) {
                print_diagnostic_section(out, "Backend Errors", backend_diagnostics);
            });
        }
        cerr << endl << "=== Backend Errors ===" << endl;
        print_text_diagnostics(cerr, backend_diagnostics);
        return 1;
    }

    backend::IntermediateCodeGenerator generator;
    auto codegen_result = generator.generate(semantic_result.decorated_ast,
                                             semantic_result.symbols,
                                             semantic_result.types);

    print_intermediate_code(cout, codegen_result.instructions);

    if (!codegen_result.ok()) {
        if (input_location != InputLocation::Milestone3) {
            write_report_file(backend_output_path, [&](ostream& out) {
                print_intermediate_code(out, codegen_result.instructions);
                print_diagnostic_section(out, "Backend Errors", codegen_result.diagnostics);
            });
        }
        cerr << endl << "=== Backend Errors ===" << endl;
        print_text_diagnostics(cerr, codegen_result.diagnostics);
        return 1;
    }

    backend::Interpreter interpreter;
    auto interpreter_result = interpreter.execute(codegen_result.instructions);

    print_program_output(cout, interpreter_result.output);

    if (!interpreter_result.ok()) {
        if (input_location != InputLocation::Milestone3) {
            write_report_file(backend_output_path, [&](ostream& out) {
                print_intermediate_code(out, codegen_result.instructions);
                print_program_output(out, interpreter_result.output);
                print_diagnostic_section(out, "Runtime Errors", interpreter_result.diagnostics);
            });
        }
        cerr << endl << "=== Runtime Errors ===" << endl;
        print_text_diagnostics(cerr, interpreter_result.diagnostics);
        return 1;
    }

    if (input_location != InputLocation::Milestone3) {
        write_report_file(backend_output_path, [&](ostream& out) {
            print_intermediate_code(out, codegen_result.instructions);
            print_program_output(out, interpreter_result.output);
        });
    }

    return 0;
}
