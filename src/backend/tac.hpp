#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>

#include "runtime_value.hpp"

namespace backend {

enum class OpCode {
    Lit,
    Lod,
    Sto,
    Lda,
    Ldi,
    Sti,
    Cal,
    Int,
    Jmp,
    Jpc,
    Opr,
    Ret,
    Chk,
    Dup,
    Pop
};

enum class OprCode {
    Neg = 1,
    Add = 2,
    Sub = 3,
    Mul = 4,
    Div = 5,
    Mod = 6,
    Eql = 7,
    Neq = 8,
    Lss = 9,
    Geq = 10,
    Gtr = 11,
    Leq = 12,
    Wrt = 13,
    Wrtln = 14
};

struct Instruction {
    OpCode opcode = OpCode::Ret;
    int level = 0;
    int argument = 0;
    RuntimeValue literal_value;
    bool has_literal_value = false;
    std::string comment;

    Instruction() = default;
    Instruction(OpCode opcode, int level, int argument);
    Instruction(OpCode opcode, int level, int argument, std::string comment);

    static Instruction literal(RuntimeValue value, int level = 0);
    static Instruction literal(RuntimeValue value, int level, std::string comment);
};

std::string to_string(OpCode opcode);
std::string to_string(OprCode opcode);
std::string format_instruction(std::size_t line, const Instruction& instruction);
std::ostream& operator<<(std::ostream& os, const Instruction& instruction);

}
