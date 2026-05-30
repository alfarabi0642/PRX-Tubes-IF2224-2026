#include "tac.hpp"

#include <ostream>
#include <sstream>
#include <utility>

namespace backend {

Instruction::Instruction(OpCode opcode, int level, int argument)
    : opcode(opcode), level(level), argument(argument) {
    if (opcode == OpCode::Lit) {
        literal_value = RuntimeValue::integer(argument);
        has_literal_value = true;
    }
}

Instruction::Instruction(OpCode opcode, int level, int argument, std::string comment)
    : opcode(opcode), level(level), argument(argument), comment(std::move(comment)) {
    if (opcode == OpCode::Lit) {
        literal_value = RuntimeValue::integer(argument);
        has_literal_value = true;
    }
}

Instruction Instruction::literal(RuntimeValue value, int level) {
    Instruction instruction(OpCode::Lit, level, 0);
    instruction.literal_value = std::move(value);
    instruction.has_literal_value = true;
    return instruction;
}

Instruction Instruction::literal(RuntimeValue value, int level, std::string comment) {
    Instruction instruction = literal(std::move(value), level);
    instruction.comment = std::move(comment);
    return instruction;
}

std::string to_string(OpCode opcode) {
    switch (opcode) {
        case OpCode::Lit: return "LIT";
        case OpCode::Lod: return "LOD";
        case OpCode::Sto: return "STO";
        case OpCode::Cal: return "CAL";
        case OpCode::Int: return "INT";
        case OpCode::Jmp: return "JMP";
        case OpCode::Jpc: return "JPC";
        case OpCode::Opr: return "OPR";
        case OpCode::Ret: return "RET";
    }
    return "UNKNOWN";
}

std::string to_string(OprCode opcode) {
    switch (opcode) {
        case OprCode::Neg: return "NEG";
        case OprCode::Add: return "ADD";
        case OprCode::Sub: return "SUB";
        case OprCode::Mul: return "MUL";
        case OprCode::Div: return "DIV";
        case OprCode::Mod: return "MOD";
        case OprCode::Eql: return "EQL";
        case OprCode::Neq: return "NEQ";
        case OprCode::Lss: return "LSS";
        case OprCode::Geq: return "GEQ";
        case OprCode::Gtr: return "GTR";
        case OprCode::Leq: return "LEQ";
        case OprCode::Wrt: return "WRT";
        case OprCode::Wrtln: return "WRTLN";
    }
    return "UNKNOWN";
}

std::string format_instruction(std::size_t line, const Instruction& instruction) {
    std::ostringstream out;
    out << line << ' ' << to_string(instruction.opcode);
    if (instruction.opcode != OpCode::Ret) {
        out << ' ' << instruction.level << ' ';
        if (instruction.opcode == OpCode::Lit && instruction.has_literal_value) {
            out << instruction.literal_value.to_code_literal();
        } else {
            out << instruction.argument;
        }
    }
    if (!instruction.comment.empty()) {
        out << " ; " << instruction.comment;
    }
    return out.str();
}

std::ostream& operator<<(std::ostream& os, const Instruction& instruction) {
    os << to_string(instruction.opcode);
    if (instruction.opcode != OpCode::Ret) {
        os << ' ' << instruction.level << ' ';
        if (instruction.opcode == OpCode::Lit && instruction.has_literal_value) {
            os << instruction.literal_value.to_code_literal();
        } else {
            os << instruction.argument;
        }
    }
    if (!instruction.comment.empty()) {
        os << " ; " << instruction.comment;
    }
    return os;
}

}
