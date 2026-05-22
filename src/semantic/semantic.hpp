#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "../parser/parse_tree.hpp"

enum class SemanticTypeKind {
    None,
    Error,
    Integer,
    Real,
    Char,
    Boolean,
    String,
    Subrange,
    Enumerated,
    Array,
    Record
};

struct SemanticType {
    SemanticTypeKind kind = SemanticTypeKind::None;
    std::string name;
    int base_type = 0;
    int ref = 0;
    int low = 0;
    int high = 0;
    int length = 0;
    bool anonymous = false;
    std::unordered_map<std::string, int> fields;
};

struct TabEntry {
    std::string identifier;
    std::string obj;
    int type = 0;
    int ref = 0;
    int nrm = 1;
    int lev = 0;
    int adr = 0;
    int link = 0;
    bool initialized = false;
    std::string value;
};

struct BTabEntry {
    int last = 0;
    int lpar = 0;
    int psze = 0;
    int vsze = 0;
};

struct ATabEntry {
    int xtyp = 0;
    int etyp = 0;
    int eref = 0;
    int low = 0;
    int high = 0;
    int elsz = 1;
    int size = 0;
};

struct DecoratedAstNode {
    std::string label;
    int type = 0;
    int tab_index = 0;
    int level = 0;
    int block_index = 0;
    std::vector<std::shared_ptr<DecoratedAstNode>> children;

    explicit DecoratedAstNode(std::string label);
    void add_child(const std::shared_ptr<DecoratedAstNode>& child);
    void print(std::ostream& os, const std::vector<SemanticType>& types) const;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    std::shared_ptr<DecoratedAstNode> analyze(const std::shared_ptr<ParseTreeNode>& root);
    void print_report(std::ostream& os) const;
    const std::vector<std::string>& get_errors() const;

private:
    struct ValueInfo {
        int type = 0;
        bool is_constant = false;
        bool initialized = false;
        int tab_index = 0;
        int int_value = 0;
        double real_value = 0.0;
        char char_value = '\0';
        bool bool_value = false;
        int length = 0;
        std::string string_value;
    };

    std::vector<TabEntry> tab;
    std::vector<BTabEntry> btab;
    std::vector<ATabEntry> atab;
    std::vector<SemanticType> types;
    std::vector<std::unordered_map<std::string, int>> scopes;
    std::vector<int> scope_last;
    std::vector<int> display;
    std::vector<int> next_address;
    std::vector<std::string> errors;
    std::shared_ptr<DecoratedAstNode> decorated_root;

    int integer_type = 0;
    int real_type = 0;
    int char_type = 0;
    int boolean_type = 0;
    int string_type = 0;

    static std::string lowercase(std::string value);
    static bool is_node(const std::shared_ptr<ParseTreeNode>& node, const std::string& name);
    static bool terminal_is(const std::shared_ptr<ParseTreeNode>& node, const std::string& token_name);
    static std::string terminal_token(const std::shared_ptr<ParseTreeNode>& node);
    static std::string terminal_value(const std::shared_ptr<ParseTreeNode>& node);
    static std::shared_ptr<ParseTreeNode> first_child(const std::shared_ptr<ParseTreeNode>& node,
                                                      const std::string& name);

    void initialize_predefined();
    int add_type(const SemanticType& type);
    std::string type_name(int type_index) const;
    int type_size(int type_index) const;

    int raw_add_symbol(const std::string& id, const std::string& obj, int type,
                       int ref, int nrm, int lev, int adr,
                       bool initialized = true, const std::string& value = "");
    int add_symbol(const std::string& id, const std::string& obj, int type,
                   int ref = 0, int nrm = 1, bool initialized = false,
                   const std::string& value = "");
    int lookup(const std::string& id) const;
    int lookup_current_scope(const std::string& id) const;
    void push_scope(int block_index);
    void pop_scope();
    void semantic_error(const std::string& message);

    std::shared_ptr<DecoratedAstNode> visit_program(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_declaration_part(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_const_declaration(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_type_declaration(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_var_declaration(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_subprogram_declaration(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_block(const std::shared_ptr<ParseTreeNode>& node, bool creates_scope);
    std::shared_ptr<DecoratedAstNode> visit_compound_statement(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_statement_list(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_statement(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_assignment(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_if(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_while(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_repeat(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_for(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_case(const std::shared_ptr<ParseTreeNode>& node);
    std::shared_ptr<DecoratedAstNode> visit_call(const std::shared_ptr<ParseTreeNode>& node, ValueInfo* out = nullptr);

    int resolve_type_node(const std::shared_ptr<ParseTreeNode>& node);
    int resolve_array_type(const std::shared_ptr<ParseTreeNode>& node);
    int resolve_range_type(const std::shared_ptr<ParseTreeNode>& node);
    int resolve_enumerated_type(const std::shared_ptr<ParseTreeNode>& node);
    int resolve_record_type(const std::shared_ptr<ParseTreeNode>& node);
    std::vector<std::string> collect_identifier_list(const std::shared_ptr<ParseTreeNode>& node) const;

    ValueInfo eval_constant(const std::shared_ptr<ParseTreeNode>& node);
    ValueInfo eval_expression(const std::shared_ptr<ParseTreeNode>& node);
    ValueInfo eval_simple_expression(const std::shared_ptr<ParseTreeNode>& node);
    ValueInfo eval_term(const std::shared_ptr<ParseTreeNode>& node);
    ValueInfo eval_factor(const std::shared_ptr<ParseTreeNode>& node);
    ValueInfo eval_variable(const std::shared_ptr<ParseTreeNode>& node, bool as_assignment_target = false);
    ValueInfo value_from_terminal(const std::shared_ptr<ParseTreeNode>& node);

    bool compatible(int left_type, int right_type) const;
    bool assignment_compatible(int target_type, const ValueInfo& value) const;
    bool numeric(int type) const;
    bool simple_non_real(int type) const;
};
