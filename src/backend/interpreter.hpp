#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "runtime_stack.hpp"
#include "tac.hpp"

namespace backend {

struct InterpreterOptions {
    std::size_t max_steps = 100000;
    std::size_t max_frames = 1000;
};

struct InterpreterResult {
    bool success = false;
    std::string output;
    std::vector<std::string> diagnostics;
    std::size_t executed_instructions = 0;

    bool ok() const;
};

class Interpreter {
public:
    explicit Interpreter(InterpreterOptions options = InterpreterOptions{});

    InterpreterResult execute(const std::vector<Instruction>& code);

private:
    InterpreterOptions options;
};

}
