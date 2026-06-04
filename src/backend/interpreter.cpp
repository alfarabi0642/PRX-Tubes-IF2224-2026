#include "interpreter.hpp"

#include <cmath>
#include <sstream>
#include <string>

namespace backend {

namespace {

void add_diagnostic(InterpreterResult* result, std::size_t ip, const std::string& message) {
    std::ostringstream out;
    out << "Runtime error at instruction " << ip << ": " << message;
    result->diagnostics.push_back(out.str());
    result->success = false;
}

bool validate_target(int target,
                     std::size_t code_size,
                     std::size_t ip,
                     InterpreterResult* result) {
    if (target >= 0 && static_cast<std::size_t>(target) < code_size) {
        return true;
    }

    std::ostringstream out;
    out << "Invalid jump target " << target << ".";
    add_diagnostic(result, ip, out.str());
    return false;
}

bool pop_operand(RuntimeStack* stack,
                 RuntimeValue* value,
                 InterpreterResult* result,
                 std::size_t ip) {
    std::string error;
    if (stack->pop(value, &error)) {
        return true;
    }

    add_diagnostic(result, ip, error);
    return false;
}

bool require_frame_operand(const Instruction& instruction,
                           std::size_t ip,
                           InterpreterResult* result,
                           const char* opcode) {
    if (instruction.level < 0) {
        std::ostringstream out;
        out << opcode << " lexical level cannot be negative.";
        add_diagnostic(result, ip, out.str());
        return false;
    }
    if (instruction.argument < 0) {
        std::ostringstream out;
        out << opcode << " address cannot be negative.";
        add_diagnostic(result, ip, out.str());
        return false;
    }
    return true;
}

bool to_ordinal(const RuntimeValue& value, int* out) {
    switch (value.kind) {
        case RuntimeValueKind::Integer:
            *out = value.int_value;
            return true;
        case RuntimeValueKind::Boolean:
            *out = value.bool_value ? 1 : 0;
            return true;
        case RuntimeValueKind::Char:
            *out = static_cast<unsigned char>(value.char_value);
            return true;
        case RuntimeValueKind::Real:
        case RuntimeValueKind::String:
        case RuntimeValueKind::Empty:
            return false;
    }
    return false;
}

bool to_absolute_address(const RuntimeValue& value,
                         std::size_t* out,
                         InterpreterResult* result,
                         std::size_t ip) {
    int address = 0;
    if (!to_ordinal(value, &address) || address < 0) {
        add_diagnostic(result, ip, "Indirect memory instruction requires a non-negative address.");
        return false;
    }
    *out = static_cast<std::size_t>(address);
    return true;
}

struct PendingCall {
    bool active = false;
    int static_link = 0;
    int dynamic_link = 0;
    int return_address = 0;
};

bool is_numeric(const RuntimeValue& value) {
    return value.kind == RuntimeValueKind::Integer || value.kind == RuntimeValueKind::Real;
}

bool to_number(const RuntimeValue& value, double* out) {
    if (value.kind == RuntimeValueKind::Integer) {
        *out = value.int_value;
        return true;
    }
    if (value.kind == RuntimeValueKind::Real) {
        *out = value.real_value;
        return true;
    }
    return false;
}

bool to_condition(const RuntimeValue& value, bool* out) {
    switch (value.kind) {
        case RuntimeValueKind::Boolean:
            *out = value.bool_value;
            return true;
        case RuntimeValueKind::Integer:
            *out = value.int_value != 0;
            return true;
        case RuntimeValueKind::Real:
            *out = value.real_value != 0.0;
            return true;
        case RuntimeValueKind::Char:
        case RuntimeValueKind::String:
        case RuntimeValueKind::Empty:
            return false;
    }
    return false;
}

bool is_zero(const RuntimeValue& value) {
    return (value.kind == RuntimeValueKind::Integer && value.int_value == 0)
        || (value.kind == RuntimeValueKind::Real && value.real_value == 0.0);
}

RuntimeValue numeric_result(const RuntimeValue& left, double value) {
    if (left.kind == RuntimeValueKind::Integer) {
        return RuntimeValue::integer(static_cast<int>(value));
    }
    return RuntimeValue::real(value);
}

RuntimeValue numeric_result(const RuntimeValue& left, const RuntimeValue& right, double value) {
    if (left.kind == RuntimeValueKind::Integer && right.kind == RuntimeValueKind::Integer) {
        return RuntimeValue::integer(static_cast<int>(value));
    }
    return RuntimeValue::real(value);
}

bool same_value(const RuntimeValue& left, const RuntimeValue& right, bool* out) {
    if (is_numeric(left) && is_numeric(right)) {
        double left_value = 0.0;
        double right_value = 0.0;
        to_number(left, &left_value);
        to_number(right, &right_value);
        *out = left_value == right_value;
        return true;
    }
    if (left.kind != right.kind) {
        return false;
    }

    switch (left.kind) {
        case RuntimeValueKind::Empty:
            *out = true;
            return true;
        case RuntimeValueKind::Char:
            *out = left.char_value == right.char_value;
            return true;
        case RuntimeValueKind::Boolean:
            *out = left.bool_value == right.bool_value;
            return true;
        case RuntimeValueKind::String:
            *out = left.string_value == right.string_value;
            return true;
        case RuntimeValueKind::Integer:
        case RuntimeValueKind::Real:
            break;
    }
    return false;
}

bool execute_unary_op(RuntimeStack* stack,
                      OprCode op,
                      InterpreterResult* result,
                      std::size_t ip) {
    RuntimeValue value;
    if (!pop_operand(stack, &value, result, ip)) {
        return false;
    }
    if (!is_numeric(value)) {
        add_diagnostic(result, ip, "Unary operator requires a numeric operand.");
        return false;
    }

    double numeric = 0.0;
    to_number(value, &numeric);
    switch (op) {
        case OprCode::Neg:
            stack->push(numeric_result(value, -numeric));
            return true;
        default:
            add_diagnostic(result, ip, "Unsupported unary OPR code.");
            return false;
    }
}

bool execute_binary_op(RuntimeStack* stack,
                       OprCode op,
                       InterpreterResult* result,
                       std::size_t ip) {
    RuntimeValue right;
    RuntimeValue left;
    if (!pop_operand(stack, &right, result, ip) || !pop_operand(stack, &left, result, ip)) {
        return false;
    }

    if (op == OprCode::Eql || op == OprCode::Neq) {
        bool equal = false;
        if (!same_value(left, right, &equal)) {
            add_diagnostic(result, ip, "Equality comparison requires compatible operands.");
            return false;
        }
        stack->push(RuntimeValue::boolean(op == OprCode::Eql ? equal : !equal));
        return true;
    }

    if (!is_numeric(left) || !is_numeric(right)) {
        add_diagnostic(result, ip, "Arithmetic and ordered comparison operators require numeric operands.");
        return false;
    }

    double left_value = 0.0;
    double right_value = 0.0;
    to_number(left, &left_value);
    to_number(right, &right_value);

    switch (op) {
        case OprCode::Add:
            stack->push(numeric_result(left, right, left_value + right_value));
            return true;
        case OprCode::Sub:
            stack->push(numeric_result(left, right, left_value - right_value));
            return true;
        case OprCode::Mul:
            stack->push(numeric_result(left, right, left_value * right_value));
            return true;
        case OprCode::Div:
            if (is_zero(right)) {
                add_diagnostic(result, ip, "Division by zero.");
                return false;
            }
            stack->push(numeric_result(left, right, left_value / right_value));
            return true;
        case OprCode::Mod:
            if (left.kind != RuntimeValueKind::Integer || right.kind != RuntimeValueKind::Integer) {
                add_diagnostic(result, ip, "Modulo requires integer operands.");
                return false;
            }
            if (right.int_value == 0) {
                add_diagnostic(result, ip, "Modulo by zero.");
                return false;
            }
            stack->push(RuntimeValue::integer(left.int_value % right.int_value));
            return true;
        case OprCode::Lss:
            stack->push(RuntimeValue::boolean(left_value < right_value));
            return true;
        case OprCode::Geq:
            stack->push(RuntimeValue::boolean(left_value >= right_value));
            return true;
        case OprCode::Gtr:
            stack->push(RuntimeValue::boolean(left_value > right_value));
            return true;
        case OprCode::Leq:
            stack->push(RuntimeValue::boolean(left_value <= right_value));
            return true;
        case OprCode::Neg:
        case OprCode::Eql:
        case OprCode::Neq:
        case OprCode::Wrt:
        case OprCode::Wrtln:
            break;
    }

    add_diagnostic(result, ip, "Unsupported binary OPR code.");
    return false;
}

bool execute_write_op(RuntimeStack* stack,
                      OprCode op,
                      InterpreterResult* result,
                      std::size_t ip) {
    RuntimeValue value;
    if (!pop_operand(stack, &value, result, ip)) {
        return false;
    }

    result->output += value.to_string();
    if (op == OprCode::Wrtln) {
        result->output += '\n';
    }
    return true;
}

bool execute_opr(RuntimeStack* stack,
                 const Instruction& instruction,
                 InterpreterResult* result,
                 std::size_t ip) {
    const OprCode op = static_cast<OprCode>(instruction.argument);
    switch (op) {
        case OprCode::Neg:
            return execute_unary_op(stack, op, result, ip);
        case OprCode::Add:
        case OprCode::Sub:
        case OprCode::Mul:
        case OprCode::Div:
        case OprCode::Mod:
        case OprCode::Eql:
        case OprCode::Neq:
        case OprCode::Lss:
        case OprCode::Geq:
        case OprCode::Gtr:
        case OprCode::Leq:
            return execute_binary_op(stack, op, result, ip);
        case OprCode::Wrt:
        case OprCode::Wrtln:
            return execute_write_op(stack, op, result, ip);
    }

    std::ostringstream out;
    out << "Unknown OPR code " << instruction.argument << ".";
    add_diagnostic(result, ip, out.str());
    return false;
}

} // namespace

bool InterpreterResult::ok() const {
    return success && diagnostics.empty();
}

Interpreter::Interpreter(InterpreterOptions options) : options(options) {}

InterpreterResult Interpreter::execute(const std::vector<Instruction>& code) {
    InterpreterResult result;
    if (code.empty()) {
        result.diagnostics.push_back("Interpreter skipped: intermediate code is empty.");
        return result;
    }

    RuntimeStack stack(options.max_frames);
    std::size_t ip = 0;
    PendingCall pending_call;

    while (true) {
        if (result.executed_instructions >= options.max_steps) {
            add_diagnostic(&result, ip, "Maximum interpreter step count exceeded.");
            return result;
        }
        if (ip >= code.size()) {
            add_diagnostic(&result, ip, "Instruction pointer is out of bounds.");
            return result;
        }

        const Instruction& instruction = code[ip];
        const std::size_t current_ip = ip;
        ++result.executed_instructions;
        ip = current_ip + 1;

        switch (instruction.opcode) {
            case OpCode::Lit:
                stack.push(instruction.has_literal_value
                    ? instruction.literal_value
                    : RuntimeValue::integer(instruction.argument));
                break;

            case OpCode::Lod: {
                if (!require_frame_operand(instruction, current_ip, &result, "LOD")) {
                    return result;
                }

                RuntimeValue value;
                std::string error;
                if (!stack.load(static_cast<std::size_t>(instruction.level),
                                static_cast<std::size_t>(instruction.argument),
                                &value,
                                &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                stack.push(value);
                break;
            }

            case OpCode::Sto: {
                if (!require_frame_operand(instruction, current_ip, &result, "STO")) {
                    return result;
                }

                RuntimeValue value;
                if (!pop_operand(&stack, &value, &result, current_ip)) {
                    return result;
                }

                std::string error;
                if (!stack.store(static_cast<std::size_t>(instruction.level),
                                 static_cast<std::size_t>(instruction.argument),
                                 value,
                                 &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                break;
            }

            case OpCode::Lda: {
                if (!require_frame_operand(instruction, current_ip, &result, "LDA")) {
                    return result;
                }

                std::size_t address = 0;
                std::string error;
                if (!stack.address(static_cast<std::size_t>(instruction.level),
                                   static_cast<std::size_t>(instruction.argument),
                                   &address,
                                   &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                stack.push(RuntimeValue::integer(static_cast<int>(address)));
                break;
            }

            case OpCode::Ldi: {
                RuntimeValue address_value;
                if (!pop_operand(&stack, &address_value, &result, current_ip)) {
                    return result;
                }

                std::size_t address = 0;
                if (!to_absolute_address(address_value, &address, &result, current_ip)) {
                    return result;
                }

                RuntimeValue value;
                std::string error;
                if (!stack.load_absolute(address, &value, &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                stack.push(value);
                break;
            }

            case OpCode::Sti: {
                RuntimeValue value;
                RuntimeValue address_value;
                if (!pop_operand(&stack, &value, &result, current_ip) ||
                    !pop_operand(&stack, &address_value, &result, current_ip)) {
                    return result;
                }

                std::size_t address = 0;
                if (!to_absolute_address(address_value, &address, &result, current_ip)) {
                    return result;
                }

                std::string error;
                if (!stack.store_absolute(address, value, &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                break;
            }

            case OpCode::Cal: {
                if (instruction.level < 0) {
                    add_diagnostic(&result, current_ip, "CAL lexical level cannot be negative.");
                    return result;
                }
                if (!validate_target(instruction.argument, code.size(), current_ip, &result)) {
                    return result;
                }
                if (pending_call.active) {
                    add_diagnostic(&result, current_ip, "Nested CAL without callee frame allocation.");
                    return result;
                }

                std::string error;
                if (!stack.static_link_target(static_cast<std::size_t>(instruction.level),
                                              &pending_call.static_link,
                                              &error) ||
                    !stack.current_frame_index(&pending_call.dynamic_link, &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                pending_call.active = true;
                pending_call.return_address = static_cast<int>(ip);
                ip = static_cast<std::size_t>(instruction.argument);
                break;
            }

            case OpCode::Int: {
                if (instruction.level < 0) {
                    add_diagnostic(&result, current_ip, "INT parameter slot count cannot be negative.");
                    return result;
                }
                if (instruction.argument < 0) {
                    add_diagnostic(&result, current_ip, "INT frame size cannot be negative.");
                    return result;
                }

                std::string error;
                const int static_link = pending_call.active ? pending_call.static_link : 0;
                const int dynamic_link = pending_call.active ? pending_call.dynamic_link : 0;
                const int return_address = pending_call.active ? pending_call.return_address : 0;
                if (!stack.push_frame(static_cast<std::size_t>(instruction.argument),
                                      static_cast<std::size_t>(instruction.level),
                                      static_link,
                                      dynamic_link,
                                      return_address,
                                      &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                pending_call = PendingCall{};
                break;
            }

            case OpCode::Jmp:
                if (!validate_target(instruction.argument, code.size(), current_ip, &result)) {
                    return result;
                }
                ip = static_cast<std::size_t>(instruction.argument);
                break;

            case OpCode::Jpc: {
                if (!validate_target(instruction.argument, code.size(), current_ip, &result)) {
                    return result;
                }

                RuntimeValue value;
                if (!pop_operand(&stack, &value, &result, current_ip)) {
                    return result;
                }

                bool condition = false;
                if (!to_condition(value, &condition)) {
                    add_diagnostic(&result, current_ip,
                        "JPC condition requires boolean, integer, or real operand.");
                    return result;
                }
                if (!condition) {
                    ip = static_cast<std::size_t>(instruction.argument);
                }
                break;
            }

            case OpCode::Opr:
                if (!execute_opr(&stack, instruction, &result, current_ip)) {
                    return result;
                }
                break;

            case OpCode::Ret: {
                if (instruction.argument != 0 && instruction.argument != 1) {
                    add_diagnostic(&result, current_ip, "RET argument must be 0 or 1.");
                    return result;
                }
                if (stack.frame_count() <= 1) {
                    result.success = true;
                    return result;
                }

                RuntimeValue return_value;
                if (instruction.argument == 1 &&
                    !pop_operand(&stack, &return_value, &result, current_ip)) {
                    return result;
                }

                int return_address = 0;
                std::string error;
                if (!stack.current_return_address(&return_address, &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                if (!stack.pop_frame(&error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                if (instruction.argument == 1) {
                    stack.push(return_value);
                }
                if (!validate_target(return_address, code.size(), current_ip, &result)) {
                    return result;
                }
                ip = static_cast<std::size_t>(return_address);
                break;
            }

            case OpCode::Chk: {
                if (instruction.level > instruction.argument) {
                    add_diagnostic(&result, current_ip, "CHK lower bound is greater than upper bound.");
                    return result;
                }

                RuntimeValue index_value;
                if (!pop_operand(&stack, &index_value, &result, current_ip)) {
                    return result;
                }

                int index = 0;
                if (!to_ordinal(index_value, &index)) {
                    add_diagnostic(&result, current_ip, "Array index must be ordinal.");
                    return result;
                }
                if (index < instruction.level || index > instruction.argument) {
                    std::ostringstream out;
                    out << "Array index " << index << " is outside bounds "
                        << instruction.level << ".." << instruction.argument << ".";
                    add_diagnostic(&result, current_ip, out.str());
                    return result;
                }
                stack.push(RuntimeValue::integer(index));
                break;
            }

            case OpCode::Dup: {
                RuntimeValue value;
                std::string error;
                if (!stack.peek(&value, &error)) {
                    add_diagnostic(&result, current_ip, error);
                    return result;
                }
                stack.push(value);
                break;
            }

            case OpCode::Pop: {
                RuntimeValue discarded;
                if (!pop_operand(&stack, &discarded, &result, current_ip)) {
                    return result;
                }
                break;
            }
        }
    }

    return result;
}

}
