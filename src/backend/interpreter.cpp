#include "interpreter.hpp"

namespace backend {

bool InterpreterResult::ok() const {
    return success && diagnostics.empty();
}

Interpreter::Interpreter(InterpreterOptions options) : options(options) {}

InterpreterResult Interpreter::execute(const std::vector<Instruction>& code) {
    (void)options;

    InterpreterResult result;
    if (code.empty()) {
        result.diagnostics.push_back("Interpreter skipped: intermediate code is empty.");
        return result;
    }

    result.diagnostics.push_back("TODO: Interpreter execution loop is not implemented yet.");
    return result;
}

}
