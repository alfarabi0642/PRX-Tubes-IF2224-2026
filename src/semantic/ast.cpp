#include "ast.hpp"

#include <memory>

namespace semantic {

// Create AST node
AstNode::AstNode(AstKind kind) : kind(kind) {}

AstNode::AstNode(AstKind kind, SourceLocation location)
    : kind(kind), location(location) {}

// Attach child node
AstNodePtr AstNode::add_child(AstNodePtr child) {
    if (child) {
        children.push_back(child);
    }
    return child;
}

// Allocate AST node
AstNodePtr make_ast(AstKind kind) {
    return std::make_shared<AstNode>(kind);
}

AstNodePtr make_ast(AstKind kind, SourceLocation location) {
    return std::make_shared<AstNode>(kind, location);
}

// Format AST kind
std::string ast_kind_name(AstKind kind) {
    switch (kind) {
        case AstKind::Program: return "Program";
        case AstKind::DeclarationPart: return "DeclarationPart";
        case AstKind::ConstDecl: return "ConstDecl";
        case AstKind::TypeDecl: return "TypeDecl";
        case AstKind::VarDecl: return "VarDecl";
        case AstKind::ProcedureDecl: return "ProcedureDecl";
        case AstKind::FunctionDecl: return "FunctionDecl";
        case AstKind::ParameterGroup: return "ParameterGroup";
        case AstKind::CompoundStatement: return "CompoundStatement";
        case AstKind::EmptyStatement: return "EmptyStatement";
        case AstKind::AssignStatement: return "AssignStatement";
        case AstKind::IfStatement: return "IfStatement";
        case AstKind::CaseStatement: return "CaseStatement";
        case AstKind::CaseBranch: return "CaseBranch";
        case AstKind::WhileStatement: return "WhileStatement";
        case AstKind::RepeatStatement: return "RepeatStatement";
        case AstKind::ForStatement: return "ForStatement";
        case AstKind::Call: return "Call";
        case AstKind::ParameterList: return "ParameterList";
        case AstKind::Variable: return "Variable";
        case AstKind::IndexComponent: return "IndexComponent";
        case AstKind::FieldComponent: return "FieldComponent";
        case AstKind::Literal: return "Literal";
        case AstKind::UnaryOp: return "UnaryOp";
        case AstKind::BinaryOp: return "BinaryOp";
        case AstKind::TypeRef: return "TypeRef";
        case AstKind::ArrayType: return "ArrayType";
        case AstKind::RangeType: return "RangeType";
        case AstKind::EnumType: return "EnumType";
        case AstKind::RecordType: return "RecordType";
        case AstKind::FieldDecl: return "FieldDecl";
    }
    return "Unknown";
}

}
