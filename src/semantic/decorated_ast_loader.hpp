#pragma once

#include <string>
#include <vector>

#include "ast.hpp"

namespace semantic {

struct DecoratedAstLoadResult {
    AstNodePtr root;
    std::vector<std::string> diagnostics;

    bool ok() const;
};

DecoratedAstLoadResult load_decorated_ast_file(const std::string& path);

}
