#pragma once

#include <memory>
#include <vector>
#include "../parser/parse_tree.hpp"
#include "ast.hpp"
#include "diagnostic.hpp"

namespace semantic {

struct AstBuildResult {
    AstNodePtr root;
    std::vector<Diagnostic> diagnostics;

    bool ok() const;
};

class AstBuilder {
public:
    AstBuildResult build(const std::shared_ptr<ParseTreeNode>& parse_tree_root);
};

} // namespace semantic
