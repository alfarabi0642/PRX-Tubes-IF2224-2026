#include "code_generator.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace backend {

namespace {

std::string normalize(std::string text) {
    for (char& ch : text) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return text;
}

std::string decode_pascal_quoted(const std::string& raw) {
    if (raw.size() < 2 || raw.front() != '\'' || raw.back() != '\'') {
        return raw;
    }

    std::string decoded;
    for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
        if (raw[i] == '\'' && i + 2 < raw.size() && raw[i + 1] == '\'') {
            decoded += '\'';
            ++i;
        } else {
            decoded += raw[i];
        }
    }
    return decoded;
}

class Generator {
public:
    Generator(const semantic::SymbolTable& symbols, const semantic::TypeRegistry& types)
        : symbols(symbols), types(types) {}

    CodegenResult run(const semantic::AstNodePtr& ast_root) {
        if (!ast_root) {
            result.diagnostics.push_back("Code generation skipped: decorated AST is missing.");
            return result;
        }
        if (ast_root->kind != semantic::AstKind::Program) {
            add_diagnostic(ast_root, "Expected Program AST root, got " +
                                         semantic::ast_kind_name(ast_root->kind) + ".");
            return result;
        }

        generate_program(ast_root);
        return std::move(result);
    }

private:
    CodegenResult result;
    const semantic::SymbolTable& symbols;
    const semantic::TypeRegistry& types;

    std::size_t current_line() const {
        return result.instructions.size();
    }

    std::size_t emit(Instruction instruction) {
        result.instructions.push_back(std::move(instruction));
        return result.instructions.size() - 1;
    }

    std::size_t emit_simple(OpCode opcode, int argument, std::string comment = "") {
        return emit(Instruction(opcode, 0, argument, std::move(comment)));
    }

    std::size_t emit_opr(OprCode op) {
        return emit_simple(OpCode::Opr, static_cast<int>(op), to_string(op));
    }

    void emit_literal(RuntimeValue value, std::string comment = "") {
        emit(Instruction::literal(std::move(value), 0, std::move(comment)));
    }

    void patch(std::size_t line, std::size_t target) {
        if (line >= result.instructions.size()) {
            result.diagnostics.push_back("Code generation internal error: invalid jump patch line.");
            return;
        }
        result.instructions[line].argument = static_cast<int>(target);
    }

    void add_diagnostic(const semantic::AstNodePtr& node, const std::string& message) {
        std::ostringstream out;
        out << "Code generation error";
        if (node && (node->location.line > 0 || node->location.column > 0)) {
            out << " at " << node->location.line << ':' << node->location.column;
        }
        out << ": " << message;
        result.diagnostics.push_back(out.str());
    }

    bool valid_tab_index(int index) const {
        return index >= 0 &&
               static_cast<std::size_t>(index) < symbols.tab_entries().size();
    }

    bool valid_block_index(int index) const {
        return index >= 0 &&
               static_cast<std::size_t>(index) < symbols.btab_entries().size();
    }

    bool valid_type_id(int type_id) const {
        return type_id >= 0 &&
               static_cast<std::size_t>(type_id) < types.entries().size();
    }

    semantic::TypeKind type_kind(int type_id) const {
        if (!valid_type_id(type_id)) {
            return semantic::TypeKind::None;
        }
        return types.get(type_id).kind;
    }

    int base_type(int type_id) const {
        if (!valid_type_id(type_id)) {
            return type_id;
        }

        const auto& type = types.get(type_id);
        if (type.kind == semantic::TypeKind::Subrange ||
            type.kind == semantic::TypeKind::Enumerated) {
            return type.base_type;
        }
        return type_id;
    }

    bool is_integer_like(int type_id) const {
        return base_type(type_id) == types.integer_type();
    }

    bool is_runtime_numeric(int type_id) const {
        const int base = base_type(type_id);
        return base == types.integer_type() || base == types.real_type();
    }

    const semantic::TabEntry* tab_entry_for_node(const semantic::AstNodePtr& node) {
        if (!node) {
            return nullptr;
        }

        int index = node->annotation.tab_index;
        if (!valid_tab_index(index) && !node->name.empty()) {
            index = symbols.lookup(node->name);
        }
        if (!valid_tab_index(index)) {
            add_diagnostic(node, "No semantic symbol entry is available for '" +
                                     (node->name.empty() ? std::string("<unnamed>") : node->name) + "'.");
            return nullptr;
        }

        return &symbols.tab_entry(index);
    }

    const semantic::TabEntry* iterator_entry_for_for_node(const semantic::AstNodePtr& node) {
        if (!node) {
            return nullptr;
        }

        int index = node->annotation.tab_index;
        if (!valid_tab_index(index) && !node->name.empty()) {
            index = symbols.lookup(node->name);
        }
        if (!valid_tab_index(index)) {
            add_diagnostic(node, "No semantic symbol entry is available for for-iterator '" +
                                     node->name + "'.");
            return nullptr;
        }
        return &symbols.tab_entry(index);
    }

    bool literal_from_constant(const semantic::AstNodePtr& node,
                               const semantic::TabEntry& entry,
                               RuntimeValue* out) {
        const int type_id = base_type(entry.type);
        const std::string lowered_name = normalize(entry.identifier);
        const std::string lowered_value = normalize(entry.value);

        try {
            if (type_id == types.boolean_type()) {
                *out = RuntimeValue::boolean(lowered_name == "true" || lowered_value == "true");
                return true;
            }
            if (type_id == types.integer_type()) {
                *out = RuntimeValue::integer(std::stoi(entry.value.empty() ? "0" : entry.value));
                return true;
            }
            if (type_id == types.real_type()) {
                *out = RuntimeValue::real(std::stod(entry.value.empty() ? "0" : entry.value));
                return true;
            }
            if (type_id == types.char_type()) {
                const std::string decoded = decode_pascal_quoted(entry.value);
                *out = RuntimeValue::character(decoded.empty() ? '\0' : decoded.front());
                return true;
            }
            if (type_id == types.string_type()) {
                *out = RuntimeValue::string(decode_pascal_quoted(entry.value));
                return true;
            }
        } catch (const std::exception&) {
            add_diagnostic(node, "Invalid constant value for '" + entry.identifier + "'.");
            return false;
        }

        add_diagnostic(node, "Constant '" + entry.identifier +
                             "' has unsupported type " + types.type_name(entry.type) + ".");
        return false;
    }

    bool literal_from_ast(const semantic::AstNodePtr& node, RuntimeValue* out) {
        if (!node) {
            return false;
        }

        try {
            switch (node->literal_kind) {
                case semantic::LiteralKind::Integer:
                    *out = RuntimeValue::integer(std::stoi(node->value));
                    return true;
                case semantic::LiteralKind::Real:
                    *out = RuntimeValue::real(std::stod(node->value));
                    return true;
                case semantic::LiteralKind::Char: {
                    const std::string decoded = decode_pascal_quoted(node->value);
                    *out = RuntimeValue::character(decoded.empty() ? '\0' : decoded.front());
                    return true;
                }
                case semantic::LiteralKind::String:
                    *out = RuntimeValue::string(decode_pascal_quoted(node->value));
                    return true;
                case semantic::LiteralKind::Boolean:
                    *out = RuntimeValue::boolean(normalize(node->value) == "true");
                    return true;
                case semantic::LiteralKind::IdentifierConstant: {
                    const semantic::TabEntry* entry = tab_entry_for_node(node);
                    if (!entry) {
                        return false;
                    }
                    return literal_from_constant(node, *entry, out);
                }
                case semantic::LiteralKind::None:
                    break;
            }
        } catch (const std::exception&) {
            add_diagnostic(node, "Invalid literal value '" + node->value + "'.");
            return false;
        }

        add_diagnostic(node, "Unsupported literal kind for value '" + node->value + "'.");
        return false;
    }

    bool runtime_address_for_entry(const semantic::AstNodePtr& node,
                                   const semantic::TabEntry& entry,
                                   int* address) {
        if (entry.lev != 0) {
            add_diagnostic(node, "Identifier '" + entry.identifier +
                                 "' is at lexical level " + std::to_string(entry.lev) +
                                 "; the current runtime only supports level 0 codegen.");
            return false;
        }
        if (entry.adr < 0) {
            add_diagnostic(node, "Identifier '" + entry.identifier +
                                 "' has a negative semantic address.");
            return false;
        }

        *address = runtime_address_for_symbol(entry);
        return true;
    }

    bool ensure_simple_variable(const semantic::AstNodePtr& node, const char* context) {
        if (!node || node->kind != semantic::AstKind::Variable) {
            add_diagnostic(node, std::string(context) + " requires a variable node.");
            return false;
        }
        if (!node->children.empty()) {
            add_diagnostic(node, std::string(context) +
                                 " for array indexes or record fields is not implemented yet.");
            return false;
        }
        return true;
    }

    bool emit_variable_load(const semantic::AstNodePtr& node) {
        if (!ensure_simple_variable(node, "Variable load")) {
            return false;
        }

        const semantic::TabEntry* entry = tab_entry_for_node(node);
        if (!entry) {
            return false;
        }

        if (entry->obj == semantic::SymbolObject::Constant) {
            RuntimeValue value;
            if (!literal_from_constant(node, *entry, &value)) {
                return false;
            }
            emit_literal(value, entry->identifier);
            return true;
        }

        if (entry->obj != semantic::SymbolObject::Variable &&
            entry->obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "Identifier '" + entry->identifier + "' is a " +
                                 semantic::symbol_object_name(entry->obj) +
                                 ", not a loadable variable in the MVP code generator.");
            return false;
        }

        int address = 0;
        if (!runtime_address_for_entry(node, *entry, &address)) {
            return false;
        }

        emit_simple(OpCode::Lod, address, entry->identifier);
        return true;
    }

    bool emit_assignment_to_entry(const semantic::AstNodePtr& node,
                                  const semantic::TabEntry& entry) {
        if (entry.obj != semantic::SymbolObject::Variable &&
            entry.obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "Identifier '" + entry.identifier + "' is a " +
                                 semantic::symbol_object_name(entry.obj) +
                                 ", not an assignable runtime storage target.");
            return false;
        }

        int address = 0;
        if (!runtime_address_for_entry(node, entry, &address)) {
            return false;
        }

        emit_simple(OpCode::Sto, address, entry.identifier);
        return true;
    }

    void generate_program(const semantic::AstNodePtr& node) {
        std::size_t frame_slots = kFrameHeaderSlots;
        if (valid_block_index(node->annotation.block_index)) {
            frame_slots = frame_slot_count_for_block(
                symbols.btab_entries()[static_cast<std::size_t>(node->annotation.block_index)]);
        } else if (!symbols.btab_entries().empty()) {
            frame_slots = frame_slot_count_for_block(symbols.btab_entries().front());
        } else {
            add_diagnostic(node, "Program block metadata is missing; using frame header only.");
        }

        emit_simple(OpCode::Int, static_cast<int>(frame_slots), "global frame");

        bool emitted_body = false;
        for (const auto& child : node->children) {
            if (!child) {
                continue;
            }
            if (child->kind == semantic::AstKind::DeclarationPart) {
                generate_declaration_part(child);
            }
        }
        for (const auto& child : node->children) {
            if (child && child->kind == semantic::AstKind::CompoundStatement) {
                generate_statement(child);
                emitted_body = true;
            }
        }

        if (!emitted_body) {
            add_diagnostic(node, "Program body compound statement is missing.");
        }

        emit(Instruction(OpCode::Ret, 0, 0));
    }

    void generate_declaration_part(const semantic::AstNodePtr& node) {
        for (const auto& child : node->children) {
            if (!child) {
                continue;
            }
            switch (child->kind) {
                case semantic::AstKind::ConstDecl:
                case semantic::AstKind::TypeDecl:
                case semantic::AstKind::VarDecl:
                    break;
                case semantic::AstKind::ProcedureDecl:
                case semantic::AstKind::FunctionDecl:
                    add_diagnostic(child, "Source-level procedure/function codegen is not "
                                          "implemented in Ishak's MVP slice; CAL/RET remain "
                                          "representable in the instruction model.");
                    break;
                default:
                    add_diagnostic(child, "Unsupported declaration node " +
                                          semantic::ast_kind_name(child->kind) + ".");
                    break;
            }
        }
    }

    void generate_statement(const semantic::AstNodePtr& node) {
        if (!node) {
            return;
        }

        switch (node->kind) {
            case semantic::AstKind::CompoundStatement:
                for (const auto& child : node->children) {
                    generate_statement(child);
                }
                break;
            case semantic::AstKind::EmptyStatement:
                break;
            case semantic::AstKind::AssignStatement:
                generate_assignment(node);
                break;
            case semantic::AstKind::IfStatement:
                generate_if(node);
                break;
            case semantic::AstKind::WhileStatement:
                generate_while(node);
                break;
            case semantic::AstKind::RepeatStatement:
                generate_repeat(node);
                break;
            case semantic::AstKind::ForStatement:
                generate_for(node);
                break;
            case semantic::AstKind::Call:
                generate_call_statement(node);
                break;
            case semantic::AstKind::Variable:
                generate_standalone_variable_statement(node);
                break;
            case semantic::AstKind::CaseStatement:
                add_diagnostic(node, "case statement codegen is not implemented yet.");
                break;
            default:
                add_diagnostic(node, "Unsupported statement node " +
                                     semantic::ast_kind_name(node->kind) + ".");
                break;
        }
    }

    void generate_assignment(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "Assignment statement is missing target or expression.");
            return;
        }

        const auto& target = node->children[0];
        if (!ensure_simple_variable(target, "Assignment")) {
            return;
        }

        const semantic::TabEntry* entry = tab_entry_for_node(target);
        if (!entry) {
            return;
        }

        if (!emit_expression(node->children[1])) {
            return;
        }
        emit_assignment_to_entry(target, *entry);
    }

    void generate_call_statement(const semantic::AstNodePtr& node) {
        const std::string name = normalize(node->name);
        if (name == "writeln" || name == "write") {
            emit_builtin_write(node, name == "writeln");
            return;
        }
        if (name == "readln") {
            add_diagnostic(node, "readln codegen is not implemented in the MVP backend.");
            return;
        }

        add_diagnostic(node, "Procedure/function call codegen for '" + node->name +
                             "' is not implemented yet.");
    }

    void generate_standalone_variable_statement(const semantic::AstNodePtr& node) {
        const semantic::TabEntry* entry = tab_entry_for_node(node);
        if (!entry) {
            return;
        }

        if (entry->obj == semantic::SymbolObject::Procedure ||
            entry->obj == semantic::SymbolObject::Function) {
            add_diagnostic(node, "Procedure/function call codegen for '" + node->name +
                                 "' is not implemented yet.");
            return;
        }

        add_diagnostic(node, "Standalone variable '" + node->name +
                             "' is not an executable statement.");
    }

    void emit_builtin_write(const semantic::AstNodePtr& node, bool newline) {
        if (node->children.empty()) {
            if (newline) {
                emit_literal(RuntimeValue::string(""), "empty writeln");
                emit_opr(OprCode::Wrtln);
            }
            return;
        }

        for (std::size_t i = 0; i < node->children.size(); ++i) {
            if (!emit_expression(node->children[i])) {
                return;
            }
            const bool last = i + 1 == node->children.size();
            emit_opr(newline && last ? OprCode::Wrtln : OprCode::Wrt);
        }
    }

    void generate_if(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "if statement is missing condition or then-branch.");
            return;
        }

        if (!emit_expression(node->children[0])) {
            return;
        }

        const std::size_t jpc_line = emit_simple(OpCode::Jpc, 0, "if false");
        generate_statement(node->children[1]);

        if (node->children.size() > 2) {
            const std::size_t jmp_end_line = emit_simple(OpCode::Jmp, 0, "if end");
            patch(jpc_line, current_line());
            generate_statement(node->children[2]);
            patch(jmp_end_line, current_line());
        } else {
            patch(jpc_line, current_line());
        }
    }

    void generate_while(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "while statement is missing condition or body.");
            return;
        }

        const std::size_t loop_start = current_line();
        if (!emit_expression(node->children[0])) {
            return;
        }

        const std::size_t jpc_end_line = emit_simple(OpCode::Jpc, 0, "while end");
        generate_statement(node->children[1]);
        emit_simple(OpCode::Jmp, static_cast<int>(loop_start), "while start");
        patch(jpc_end_line, current_line());
    }

    void generate_repeat(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "repeat statement is missing body or until condition.");
            return;
        }

        const std::size_t loop_start = current_line();
        generate_statement(node->children[0]);
        if (!emit_expression(node->children[1])) {
            return;
        }
        emit_simple(OpCode::Jpc, static_cast<int>(loop_start), "repeat until");
    }

    void generate_for(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 3) {
            add_diagnostic(node, "for statement is missing bounds or body.");
            return;
        }

        const semantic::TabEntry* entry = iterator_entry_for_for_node(node);
        if (!entry) {
            return;
        }
        if (entry->obj != semantic::SymbolObject::Variable &&
            entry->obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "for iterator '" + entry->identifier +
                                 "' is not an assignable variable.");
            return;
        }
        if (!is_integer_like(entry->type)) {
            add_diagnostic(node, "for iterator '" + entry->identifier +
                                 "' uses type " + types.type_name(entry->type) +
                                 "; only integer-like for loops are implemented.");
            return;
        }

        int address = 0;
        if (!runtime_address_for_entry(node, *entry, &address)) {
            return;
        }

        if (!emit_expression(node->children[0])) {
            return;
        }
        emit_simple(OpCode::Sto, address, entry->identifier);

        const bool downto = normalize(node->op) == "downtosy";
        const std::size_t loop_start = current_line();
        emit_simple(OpCode::Lod, address, entry->identifier);
        if (!emit_expression(node->children[1])) {
            return;
        }
        emit_opr(downto ? OprCode::Geq : OprCode::Leq);
        const std::size_t jpc_end_line = emit_simple(OpCode::Jpc, 0, "for end");

        generate_statement(node->children[2]);

        emit_simple(OpCode::Lod, address, entry->identifier);
        emit_literal(RuntimeValue::integer(1), "for step");
        emit_opr(downto ? OprCode::Sub : OprCode::Add);
        emit_simple(OpCode::Sto, address, entry->identifier);
        emit_simple(OpCode::Jmp, static_cast<int>(loop_start), "for start");
        patch(jpc_end_line, current_line());
    }

    bool emit_expression(const semantic::AstNodePtr& node) {
        if (!node) {
            return false;
        }

        switch (node->kind) {
            case semantic::AstKind::Literal:
                return emit_literal_expression(node);
            case semantic::AstKind::Variable:
                return emit_variable_load(node);
            case semantic::AstKind::UnaryOp:
                return emit_unary_expression(node);
            case semantic::AstKind::BinaryOp:
                return emit_binary_expression(node);
            case semantic::AstKind::Call:
                add_diagnostic(node, "Function call expression codegen for '" + node->name +
                                     "' is not implemented yet.");
                return false;
            default:
                add_diagnostic(node, "Unsupported expression node " +
                                     semantic::ast_kind_name(node->kind) + ".");
                return false;
        }
    }

    bool emit_literal_expression(const semantic::AstNodePtr& node) {
        RuntimeValue value;
        if (!literal_from_ast(node, &value)) {
            return false;
        }
        emit_literal(value);
        return true;
    }

    bool emit_unary_expression(const semantic::AstNodePtr& node) {
        if (!node || node->children.empty()) {
            add_diagnostic(node, "Unary expression is missing operand.");
            return false;
        }

        const std::string op = normalize(node->op);
        const auto& operand = node->children.front();
        if (!emit_expression(operand)) {
            return false;
        }

        if (op == "minus") {
            if (!operand_is_already_signed_numeric_literal(operand)) {
                emit_opr(OprCode::Neg);
            }
            return true;
        }
        if (op == "plus") {
            return true;
        }
        if (op == "notsy") {
            emit_literal(RuntimeValue::boolean(false), "not false");
            emit_opr(OprCode::Eql);
            return true;
        }

        add_diagnostic(node, "Unsupported unary operator '" + node->op + "'.");
        return false;
    }

    bool operand_is_already_signed_numeric_literal(const semantic::AstNodePtr& node) const {
        if (!node || node->kind != semantic::AstKind::Literal || node->value.empty()) {
            return false;
        }
        if (node->literal_kind != semantic::LiteralKind::Integer &&
            node->literal_kind != semantic::LiteralKind::Real) {
            return false;
        }
        return node->value.front() == '-';
    }

    bool emit_binary_expression(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "Binary expression is missing operand(s).");
            return false;
        }

        const std::string op = normalize(node->op);
        if (op == "andsy") {
            return emit_short_circuit_and(node);
        }
        if (op == "orsy") {
            return emit_short_circuit_or(node);
        }

        OprCode opr = OprCode::Add;
        if (!opr_for_binary_operator(node, &opr)) {
            return false;
        }

        if (!emit_expression(node->children[0]) || !emit_expression(node->children[1])) {
            return false;
        }
        emit_opr(opr);
        return true;
    }

    bool opr_for_binary_operator(const semantic::AstNodePtr& node, OprCode* out) {
        const std::string op = normalize(node->op);
        if (op == "plus") {
            *out = OprCode::Add;
            return true;
        }
        if (op == "minus") {
            *out = OprCode::Sub;
            return true;
        }
        if (op == "times") {
            *out = OprCode::Mul;
            return true;
        }
        if (op == "rdiv" || op == "idiv") {
            *out = OprCode::Div;
            return true;
        }
        if (op == "imod") {
            *out = OprCode::Mod;
            return true;
        }
        if (op == "eql") {
            *out = OprCode::Eql;
            return true;
        }
        if (op == "neq") {
            *out = OprCode::Neq;
            return true;
        }
        if (op == "lss") {
            if (!ordered_comparison_supported(node)) {
                return false;
            }
            *out = OprCode::Lss;
            return true;
        }
        if (op == "leq") {
            if (!ordered_comparison_supported(node)) {
                return false;
            }
            *out = OprCode::Leq;
            return true;
        }
        if (op == "gtr") {
            if (!ordered_comparison_supported(node)) {
                return false;
            }
            *out = OprCode::Gtr;
            return true;
        }
        if (op == "geq") {
            if (!ordered_comparison_supported(node)) {
                return false;
            }
            *out = OprCode::Geq;
            return true;
        }

        add_diagnostic(node, "Unsupported binary operator '" + node->op + "'.");
        return false;
    }

    bool ordered_comparison_supported(const semantic::AstNodePtr& node) {
        if (node->children.size() < 2) {
            return false;
        }
        const int left_type = node->children[0] ? node->children[0]->annotation.type_id : 0;
        const int right_type = node->children[1] ? node->children[1]->annotation.type_id : 0;
        if ((left_type == 0 || is_runtime_numeric(left_type)) &&
            (right_type == 0 || is_runtime_numeric(right_type))) {
            return true;
        }

        add_diagnostic(node, "Ordered comparisons for non-numeric operands are not "
                             "implemented by the current runtime OPR contract.");
        return false;
    }

    bool emit_short_circuit_and(const semantic::AstNodePtr& node) {
        if (!emit_expression(node->children[0])) {
            return false;
        }
        const std::size_t left_false = emit_simple(OpCode::Jpc, 0, "and false");

        if (!emit_expression(node->children[1])) {
            return false;
        }
        const std::size_t right_false = emit_simple(OpCode::Jpc, 0, "and false");

        emit_literal(RuntimeValue::boolean(true), "and true");
        const std::size_t end_jump = emit_simple(OpCode::Jmp, 0, "and end");

        const std::size_t false_line = current_line();
        patch(left_false, false_line);
        patch(right_false, false_line);
        emit_literal(RuntimeValue::boolean(false), "and false");
        patch(end_jump, current_line());
        return true;
    }

    bool emit_short_circuit_or(const semantic::AstNodePtr& node) {
        if (!emit_expression(node->children[0])) {
            return false;
        }
        const std::size_t evaluate_right = emit_simple(OpCode::Jpc, 0, "or rhs");

        emit_literal(RuntimeValue::boolean(true), "or true");
        const std::size_t left_true_end = emit_simple(OpCode::Jmp, 0, "or end");

        patch(evaluate_right, current_line());
        if (!emit_expression(node->children[1])) {
            return false;
        }
        const std::size_t right_false = emit_simple(OpCode::Jpc, 0, "or false");

        emit_literal(RuntimeValue::boolean(true), "or true");
        const std::size_t right_true_end = emit_simple(OpCode::Jmp, 0, "or end");

        patch(right_false, current_line());
        emit_literal(RuntimeValue::boolean(false), "or false");

        const std::size_t end_line = current_line();
        patch(left_true_end, end_line);
        patch(right_true_end, end_line);
        return true;
    }
};

} // namespace

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
    Generator generator(symbols, types);
    return generator.run(ast_root);
}

}
