#pragma once

#include <string>
#include <vector>

namespace semantic {

enum class DiagnosticSeverity {
    Error,
    Warning
};

struct SourceLocation {
    int line = 0;
    int column = 0;
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    SourceLocation location;
    std::string message;
};

inline bool has_errors(const std::vector<Diagnostic>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

} // namespace semantic
