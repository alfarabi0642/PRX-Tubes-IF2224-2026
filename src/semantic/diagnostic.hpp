#pragma once

#include <string>
#include <vector>

namespace semantic {

// Diagnostic severity kind
enum class DiagnosticSeverity {
    Error,
    Warning
};

// Source position
struct SourceLocation {
    int line = 0;
    int column = 0;
};

// Semantic diagnostic entry
struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    SourceLocation location;
    std::string message;
};

// Error presence check
inline bool has_errors(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

} 
