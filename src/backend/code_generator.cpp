#include "code_generator.hpp"

namespace backend {

int runtime_address_for_symbol(const semantic::TabEntry& entry) {
    return static_cast<int>(kFrameHeaderSlots) + entry.adr;
}

std::size_t frame_slot_count_for_block(const semantic::BTabEntry& block) {
    const int variable_slots = block.vsze < 0 ? 0 : block.vsze;
    return kFrameHeaderSlots + static_cast<std::size_t>(variable_slots);
}

bool CodegenResult::ok() const {
    return diagnostics.empty();
}

CodegenResult IntermediateCodeGenerator::generate(const semantic::AstNodePtr& ast_root,
                                                  const semantic::SymbolTable& symbols,
                                                  const semantic::TypeRegistry& types) {
    (void)symbols;
    (void)types;

    CodegenResult result;
    if (!ast_root) {
        result.diagnostics.push_back("Code generation skipped: decorated AST is missing.");
        return result;
    }

    result.diagnostics.push_back(
        "TODO: IntermediateCodeGenerator traversal is not implemented yet.");
    return result;
}

}
