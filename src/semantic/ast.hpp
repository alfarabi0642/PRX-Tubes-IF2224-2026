#pragma once

#include <memory>
#include <string>
#include <vector>
#include "diagnostic.hpp"

namespace semantic {

enum class AstKind {
    Program,
    DeclarationPart,
    ConstDecl,
    TypeDecl,
    VarDecl,
    ProcedureDecl,
    FunctionDecl,
    ParameterGroup,
    CompoundStatement,
    EmptyStatement,
    AssignStatement,
    IfStatement,
    CaseStatement,
    CaseBranch,
    WhileStatement,
    RepeatStatement,
    ForStatement,
    Call,
    ParameterList,
    Variable,
    IndexComponent,
    FieldComponent,
    Literal,
    UnaryOp,
    BinaryOp,
    TypeRef,
    ArrayType,
    RangeType,
    EnumType,
    RecordType,
    FieldDecl
};

enum class LiteralKind {
    None,
    Integer,
    Real,
    Char,
    String,
    Boolean,
    IdentifierConstant
};

struct AstAnnotation {
    int type_id = 0;
    int tab_index = -1;
    int block_index = 0;
    int lexical_level = 0;
    bool initialized = false;
};

struct AstNode;
using AstNodePtr = std::shared_ptr<AstNode>;

struct AstNode {
    AstKind kind;
    SourceLocation location;
    std::string name;
    std::string value;
    std::string op;
    LiteralKind literal_kind = LiteralKind::None;
    AstAnnotation annotation;
    std::vector<AstNodePtr> children;

    explicit AstNode(AstKind kind);
    AstNode(AstKind kind, SourceLocation location);
    AstNodePtr add_child(AstNodePtr child);
};

AstNodePtr make_ast(AstKind kind);
AstNodePtr make_ast(AstKind kind, SourceLocation location);
std::string ast_kind_name(AstKind kind);

} // namespace semantic
