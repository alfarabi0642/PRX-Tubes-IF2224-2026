#pragma once

#include <string>
#include <vector>

#include "../semantic/ast.hpp"
#include "../semantic/symbol_table.hpp"
#include "../semantic/types.hpp"
#include "runtime_layout.hpp"
#include "tac.hpp"

namespace backend {

struct CodegenResult {
    std::vector<Instruction> instructions;
    std::vector<std::string> diagnostics;

    bool ok() const;
};

int runtime_address_for_symbol(const semantic::TabEntry& entry);
std::size_t frame_slot_count_for_block(const semantic::BTabEntry& block);

class IntermediateCodeGenerator {
public:
    CodegenResult generate(const semantic::AstNodePtr& ast_root,
                           const semantic::SymbolTable& symbols,
                           const semantic::TypeRegistry& types);
};

}
