#pragma once

#include <ostream>
#include "ast.hpp"
#include "diagnostic.hpp"
#include "symbol_table.hpp"
#include "types.hpp"

namespace semantic {

void print_decorated_ast(std::ostream& os, const AstNodePtr& root, const TypeRegistry& types);
void print_symbol_tables(std::ostream& os, const SymbolTable& symbols, const TypeRegistry& types);
void print_diagnostics(std::ostream& os, const std::vector<Diagnostic>& diagnostics);
void print_semantic_report(std::ostream& os, const AstNodePtr& root,
                           const SymbolTable& symbols,
                           const TypeRegistry& types,
                           const std::vector<Diagnostic>& diagnostics);

} // namespace semantic
