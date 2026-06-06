#pragma once

#include <memory>
#include <vector>
#include "../parser/parse_tree.hpp"
#include "ast.hpp"
#include "diagnostic.hpp"

namespace semantic {

// AST build result
struct AstBuildResult {
    AstNodePtr root;
    std::vector<Diagnostic> diagnostics;

    bool ok() const;
};

// Parse tree converter
class AstBuilder {
public:
    AstBuildResult build(const std::shared_ptr<ParseTreeNode>& parse_tree_root);
};

} 
