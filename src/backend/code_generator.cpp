#include "code_generator.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
    struct SubprogramInfo {
        semantic::AstNodePtr declaration;
        int tab_index = -1;
        bool is_function = false;
        int label = -1;
        int block_index = -1;
        int parent_level = 0;
        int body_level = 0;
        int parameter_slots = 0;
        std::size_t frame_slots = kFrameHeaderSlots;
        int result_slot = -1;
    };

    CodegenResult result;
    const semantic::SymbolTable& symbols;
    const semantic::TypeRegistry& types;
    std::unordered_map<int, SubprogramInfo> subprograms;
    std::vector<int> subprogram_order;
    int current_level = 0;
    int current_function_tab_index = -1;
    int current_function_result_slot = -1;

    std::size_t current_line() const {
        return result.instructions.size();
    }

    std::size_t emit(Instruction instruction) {
        result.instructions.push_back(std::move(instruction));
        return result.instructions.size() - 1;
    }

    std::size_t emit_simple(OpCode opcode, int argument, std::string comment = "") {
        return emit_instruction(opcode, 0, argument, std::move(comment));
    }

    std::size_t emit_instruction(OpCode opcode,
                                 int level,
                                 int argument,
                                 std::string comment = "") {
        return emit(Instruction(opcode, level, argument, std::move(comment)));
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

    bool valid_array_index(int index) const {
        return index >= 0 &&
               static_cast<std::size_t>(index) < symbols.atab_entries().size();
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
                                   int* lexical_level,
                                   int* address) {
        const int levels_up = current_level - entry.lev;
        if (levels_up < 0) {
            add_diagnostic(node, "Identifier '" + entry.identifier +
                                 "' is declared at lexical level " + std::to_string(entry.lev) +
                                 ", which is not reachable from current level " +
                                 std::to_string(current_level) + ".");
            return false;
        }
        if (entry.adr < 0) {
            add_diagnostic(node, "Identifier '" + entry.identifier +
                                 "' has a negative semantic address.");
            return false;
        }

        *lexical_level = levels_up;
        *address = runtime_address_for_symbol(entry);
        return true;
    }

    bool is_storage_entry(const semantic::TabEntry& entry) const {
        return entry.obj == semantic::SymbolObject::Variable ||
               entry.obj == semantic::SymbolObject::Parameter ||
               entry.obj == semantic::SymbolObject::Constant ||
               entry.obj == semantic::SymbolObject::Function;
    }

    const semantic::TabEntry* storage_entry_for_variable_node(const semantic::AstNodePtr& node,
                                                              int* out_index = nullptr) {
        if (!node || node->kind != semantic::AstKind::Variable) {
            add_diagnostic(node, "Variable access requires a variable node.");
            return nullptr;
        }

        int index = node->annotation.tab_index;
        if (valid_tab_index(index) && is_storage_entry(symbols.tab_entry(index))) {
            if (out_index) {
                *out_index = index;
            }
            return &symbols.tab_entry(index);
        }

        int best = -1;
        for (std::size_t i = 0; i < symbols.tab_entries().size(); ++i) {
            const auto& entry = symbols.tab_entries()[i];
            if (entry.identifier == node->name && is_storage_entry(entry) && entry.lev <= current_level) {
                if (best < 0 || entry.lev >= symbols.tab_entry(best).lev) {
                    best = static_cast<int>(i);
                }
            }
        }
        if (!valid_tab_index(best)) {
            add_diagnostic(node, "No runtime storage symbol is available for '" + node->name + "'.");
            return nullptr;
        }

        if (out_index) {
            *out_index = best;
        }
        return &symbols.tab_entry(best);
    }

    bool emit_variable_address(const semantic::AstNodePtr& node, int* out_type = nullptr) {
        const semantic::TabEntry* entry = storage_entry_for_variable_node(node);
        if (!entry) {
            return false;
        }
        if (entry->obj != semantic::SymbolObject::Variable &&
            entry->obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "Identifier '" + entry->identifier + "' is a " +
                                 semantic::symbol_object_name(entry->obj) +
                                 ", not an addressable runtime storage target.");
            return false;
        }

        int lexical_level = 0;
        int address = 0;
        if (!runtime_address_for_entry(node, *entry, &lexical_level, &address)) {
            return false;
        }

        emit_instruction(OpCode::Lda, lexical_level, address, entry->identifier);

        int current_type = entry->type;
        for (const auto& component : node->children) {
            if (!component) {
                continue;
            }

            if (component->kind == semantic::AstKind::IndexComponent) {
                if (type_kind(current_type) != semantic::TypeKind::Array) {
                    add_diagnostic(component, "Indexed access requires an array value.");
                    return false;
                }
                const int array_ref = types.get(current_type).ref;
                if (!valid_array_index(array_ref)) {
                    add_diagnostic(component, "Array type metadata is missing.");
                    return false;
                }
                if (component->children.empty()) {
                    add_diagnostic(component, "Array index component is missing an expression.");
                    return false;
                }

                const auto& array = symbols.atab_entry(array_ref);
                if (!emit_expression(component->children.front())) {
                    return false;
                }
                emit_instruction(OpCode::Chk, array.low, array.high, "array bounds");
                if (array.low != 0) {
                    emit_literal(RuntimeValue::integer(array.low), "array low bound");
                    emit_opr(OprCode::Sub);
                }
                if (array.elsz != 1) {
                    emit_literal(RuntimeValue::integer(array.elsz), "array element size");
                    emit_opr(OprCode::Mul);
                }
                emit_opr(OprCode::Add);
                current_type = array.etyp;
            } else if (component->kind == semantic::AstKind::FieldComponent) {
                if (type_kind(current_type) != semantic::TypeKind::Record) {
                    add_diagnostic(component, "Field access requires a record value.");
                    return false;
                }

                const auto& record = types.get(current_type);
                auto found = record.fields.find(component->name);
                if (found == record.fields.end()) {
                    found = record.fields.find(normalize(component->name));
                }
                if (found == record.fields.end() || !valid_tab_index(found->second)) {
                    add_diagnostic(component, "Record field '" + component->name + "' is not available.");
                    return false;
                }

                const auto& field = symbols.tab_entry(found->second);
                if (field.adr != 0) {
                    emit_literal(RuntimeValue::integer(field.adr), field.identifier);
                    emit_opr(OprCode::Add);
                }
                current_type = field.type;
            } else {
                add_diagnostic(component, "Unsupported variable component " +
                                         semantic::ast_kind_name(component->kind) + ".");
                return false;
            }
        }

        if (out_type) {
            *out_type = current_type;
        }
        return true;
    }

    bool emit_variable_load(const semantic::AstNodePtr& node) {
        const semantic::TabEntry* entry = storage_entry_for_variable_node(node);
        if (!entry) {
            return false;
        }

        if (entry->obj == semantic::SymbolObject::Constant && node->children.empty()) {
            RuntimeValue value;
            if (!literal_from_constant(node, *entry, &value)) {
                return false;
            }
            emit_literal(value, entry->identifier);
            return true;
        }

        if (!node->children.empty()) {
            if (!emit_variable_address(node)) {
                return false;
            }
            emit_simple(OpCode::Ldi, 0, "load indirect");
            return true;
        }

        if (entry->obj != semantic::SymbolObject::Variable &&
            entry->obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "Identifier '" + entry->identifier + "' is a " +
                                 semantic::symbol_object_name(entry->obj) +
                                 ", not a loadable variable.");
            return false;
        }

        int lexical_level = 0;
        int address = 0;
        if (!runtime_address_for_entry(node, *entry, &lexical_level, &address)) {
            return false;
        }

        emit_instruction(OpCode::Lod, lexical_level, address, entry->identifier);
        return true;
    }

    bool emit_assignment_to_entry(const semantic::AstNodePtr& node,
                                  const semantic::TabEntry& entry) {
        if (node && node->annotation.tab_index == current_function_tab_index &&
            current_function_result_slot >= 0) {
            emit_instruction(OpCode::Sto, 0, current_function_result_slot, entry.identifier + " result");
            return true;
        }

        if (entry.obj != semantic::SymbolObject::Variable &&
            entry.obj != semantic::SymbolObject::Parameter) {
            add_diagnostic(node, "Identifier '" + entry.identifier + "' is a " +
                                 semantic::symbol_object_name(entry.obj) +
                                 ", not an assignable runtime storage target.");
            return false;
        }

        int lexical_level = 0;
        int address = 0;
        if (!runtime_address_for_entry(node, entry, &lexical_level, &address)) {
            return false;
        }

        emit_instruction(OpCode::Sto, lexical_level, address, entry.identifier);
        return true;
    }

    void generate_program(const semantic::AstNodePtr& node) {
        current_level = node->annotation.lexical_level;

        std::size_t frame_slots = kFrameHeaderSlots;
        if (valid_block_index(node->annotation.block_index)) {
            frame_slots = frame_slot_count_for_block(
                symbols.btab_entries()[static_cast<std::size_t>(node->annotation.block_index)]);
        } else if (!symbols.btab_entries().empty()) {
            frame_slots = frame_slot_count_for_block(symbols.btab_entries().front());
        } else {
            add_diagnostic(node, "Program block metadata is missing; using frame header only.");
        }

        emit_instruction(OpCode::Int, 0, static_cast<int>(frame_slots), "global frame");

        for (const auto& child : node->children) {
            if (child && child->kind == semantic::AstKind::DeclarationPart) {
                collect_subprograms(child);
                generate_declaration_part(child);
            }
        }

        std::size_t main_jump = 0;
        const bool has_subprograms = !subprogram_order.empty();
        if (has_subprograms) {
            main_jump = emit_simple(OpCode::Jmp, 0, "main");
        }
        for (int tab_index : subprogram_order) {
            auto found = subprograms.find(tab_index);
            if (found != subprograms.end()) {
                emit_subprogram_body(found->second);
            }
        }

        if (has_subprograms) {
            patch(main_jump, current_line());
        }

        bool emitted_body = false;
        for (const auto& child : node->children) {
            if (child && child->kind == semantic::AstKind::CompoundStatement) {
                current_level = child->annotation.lexical_level;
                generate_statement(child);
                emitted_body = true;
            }
        }

        if (!emitted_body) {
            add_diagnostic(node, "Program body compound statement is missing.");
        }

        emit(Instruction(OpCode::Ret, 0, 0));
    }

    void collect_subprograms(const semantic::AstNodePtr& node) {
        if (!node) {
            return;
        }
        for (const auto& child : node->children) {
            if (!child) {
                continue;
            }
            if (child->kind == semantic::AstKind::ProcedureDecl ||
                child->kind == semantic::AstKind::FunctionDecl) {
                const int tab_index = child->annotation.tab_index;
                if (!valid_tab_index(tab_index)) {
                    add_diagnostic(child, "Subprogram declaration is missing symbol metadata.");
                    continue;
                }

                const auto& entry = symbols.tab_entry(tab_index);
                if (!valid_block_index(entry.ref)) {
                    add_diagnostic(child, "Subprogram '" + entry.identifier +
                                         "' is missing block metadata.");
                    continue;
                }

                const auto& block = symbols.btab_entries()[static_cast<std::size_t>(entry.ref)];
                SubprogramInfo info;
                info.declaration = child;
                info.tab_index = tab_index;
                info.is_function = child->kind == semantic::AstKind::FunctionDecl;
                info.block_index = entry.ref;
                info.parent_level = child->annotation.lexical_level;
                info.body_level = child->annotation.lexical_level + 1;
                info.parameter_slots = block.psze < 0 ? 0 : block.psze;
                info.frame_slots = frame_slot_count_for_block(block);
                if (info.is_function) {
                    info.result_slot = static_cast<int>(info.frame_slots);
                    ++info.frame_slots;
                }
                subprograms[tab_index] = info;
                subprogram_order.push_back(tab_index);
            }
        }
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
                case semantic::AstKind::ProcedureDecl:
                case semantic::AstKind::FunctionDecl:
                    break;
                default:
                    add_diagnostic(child, "Unsupported declaration node " +
                                          semantic::ast_kind_name(child->kind) + ".");
                    break;
            }
        }
    }

    void emit_subprogram_body(SubprogramInfo& info) {
        info.label = static_cast<int>(current_line());
        emit_instruction(OpCode::Int,
                         info.parameter_slots,
                         static_cast<int>(info.frame_slots),
                         symbols.tab_entry(info.tab_index).identifier + " frame");

        const int saved_level = current_level;
        const int saved_function_tab_index = current_function_tab_index;
        const int saved_function_result_slot = current_function_result_slot;

        current_level = info.body_level;
        current_function_tab_index = info.is_function ? info.tab_index : -1;
        current_function_result_slot = info.is_function ? info.result_slot : -1;

        bool emitted_body = false;
        for (const auto& child : info.declaration->children) {
            if (!child) {
                continue;
            }
            if (child->kind == semantic::AstKind::DeclarationPart) {
                generate_declaration_part(child);
            } else if (child->kind == semantic::AstKind::CompoundStatement) {
                current_level = child->annotation.lexical_level;
                generate_statement(child);
                emitted_body = true;
            }
        }
        if (!emitted_body) {
            add_diagnostic(info.declaration, "Subprogram body compound statement is missing.");
        }

        if (info.is_function) {
            emit_instruction(OpCode::Lod, 0, info.result_slot,
                             symbols.tab_entry(info.tab_index).identifier + " result");
            emit(Instruction(OpCode::Ret, 0, 1));
        } else {
            emit(Instruction(OpCode::Ret, 0, 0));
        }

        current_level = saved_level;
        current_function_tab_index = saved_function_tab_index;
        current_function_result_slot = saved_function_result_slot;
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
                generate_case(node);
                break;
            default:
                add_diagnostic(node, "Unsupported statement node " +
                                     semantic::ast_kind_name(node->kind) + ".");
                break;
        }
    }

    void generate_case(const semantic::AstNodePtr& node) {
        if (!node || node->children.empty()) {
            add_diagnostic(node, "case statement is missing selector expression.");
            return;
        }
        if (!emit_expression(node->children.front())) {
            return;
        }

        std::vector<std::size_t> end_jumps;
        for (std::size_t i = 1; i < node->children.size(); ++i) {
            const auto& branch = node->children[i];
            if (!branch || branch->kind != semantic::AstKind::CaseBranch) {
                continue;
            }

            std::vector<semantic::AstNodePtr> labels;
            std::vector<semantic::AstNodePtr> statements;
            for (const auto& child : branch->children) {
                if (!child) {
                    continue;
                }
                if (child->kind == semantic::AstKind::Literal ||
                    child->kind == semantic::AstKind::UnaryOp) {
                    labels.push_back(child);
                } else {
                    statements.push_back(child);
                }
            }

            for (const auto& label : labels) {
                emit_simple(OpCode::Dup, 0, "case selector");
                if (!emit_expression(label)) {
                    return;
                }
                emit_opr(OprCode::Eql);
                const std::size_t no_match = emit_simple(OpCode::Jpc, 0, "case no match");

                emit_simple(OpCode::Pop, 0, "discard matched selector");
                for (const auto& statement : statements) {
                    generate_statement(statement);
                }
                end_jumps.push_back(emit_simple(OpCode::Jmp, 0, "case end"));
                patch(no_match, current_line());
            }
        }

        emit_simple(OpCode::Pop, 0, "discard unmatched selector");
        const std::size_t end_line = current_line();
        for (std::size_t jump : end_jumps) {
            patch(jump, end_line);
        }
    }

    void generate_assignment(const semantic::AstNodePtr& node) {
        if (!node || node->children.size() < 2) {
            add_diagnostic(node, "Assignment statement is missing target or expression.");
            return;
        }

        const auto& target = node->children[0];
        if (!target || target->kind != semantic::AstKind::Variable) {
            add_diagnostic(target, "Assignment target must be a variable.");
            return;
        }

        if (!target->children.empty()) {
            if (!emit_variable_address(target)) {
                return;
            }
            if (!emit_expression(node->children[1])) {
                return;
            }
            emit_simple(OpCode::Sti, 0, "store indirect");
            return;
        }

        const semantic::TabEntry* entry = storage_entry_for_variable_node(target);
        if (!entry || !emit_expression(node->children[1])) {
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

        emit_user_call(node, false, true);
    }

    void generate_standalone_variable_statement(const semantic::AstNodePtr& node) {
        const semantic::TabEntry* entry = tab_entry_for_node(node);
        if (!entry) {
            return;
        }

        if (entry->obj == semantic::SymbolObject::Procedure ||
            entry->obj == semantic::SymbolObject::Function) {
            emit_user_call(node, false, true);
            return;
        }

        add_diagnostic(node, "Standalone variable '" + node->name +
                             "' is not an executable statement.");
    }

    bool emit_user_call(const semantic::AstNodePtr& node,
                        bool require_function,
                        bool discard_function_result) {
        int tab_index = node ? node->annotation.tab_index : -1;
        const semantic::TabEntry* entry = tab_entry_for_node(node);
        if (!entry) {
            return false;
        }
        if (!valid_tab_index(tab_index)) {
            for (std::size_t i = 0; i < symbols.tab_entries().size(); ++i) {
                const auto& candidate = symbols.tab_entries()[i];
                if (candidate.identifier == node->name &&
                    (candidate.obj == semantic::SymbolObject::Procedure ||
                     candidate.obj == semantic::SymbolObject::Function)) {
                    tab_index = static_cast<int>(i);
                    break;
                }
            }
        }

        if (entry->obj != semantic::SymbolObject::Procedure &&
            entry->obj != semantic::SymbolObject::Function) {
            add_diagnostic(node, "Identifier '" + entry->identifier + "' is a " +
                                 semantic::symbol_object_name(entry->obj) +
                                 ", not a callable subprogram.");
            return false;
        }
        if (require_function && entry->obj != semantic::SymbolObject::Function) {
            add_diagnostic(node, "Call to '" + entry->identifier +
                                 "' is not a function expression.");
            return false;
        }
        if (!valid_tab_index(tab_index)) {
            add_diagnostic(node, "Callable '" + entry->identifier + "' is missing symbol metadata.");
            return false;
        }

        auto found = subprograms.find(tab_index);
        if (found == subprograms.end()) {
            add_diagnostic(node, "Callable '" + entry->identifier +
                                 "' has no generated subprogram body.");
            return false;
        }
        const SubprogramInfo& info = found->second;
        if (info.label < 0) {
            add_diagnostic(node, "Callable '" + entry->identifier +
                                 "' is referenced before its backend label is available.");
            return false;
        }

        for (const auto& argument : node->children) {
            if (!emit_expression(argument)) {
                return false;
            }
        }

        const int levels_up = current_level - info.parent_level;
        if (levels_up < 0) {
            add_diagnostic(node, "Callable '" + entry->identifier +
                                 "' is not reachable from current lexical level.");
            return false;
        }

        emit_instruction(OpCode::Cal, levels_up, info.label, entry->identifier);
        if (discard_function_result && entry->obj == semantic::SymbolObject::Function) {
            emit_simple(OpCode::Pop, 0, "discard function result");
        }
        return true;
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

        int lexical_level = 0;
        int address = 0;
        if (!runtime_address_for_entry(node, *entry, &lexical_level, &address)) {
            return;
        }

        if (!emit_expression(node->children[0])) {
            return;
        }
        emit_instruction(OpCode::Sto, lexical_level, address, entry->identifier);

        const bool downto = normalize(node->op) == "downtosy";
        const std::size_t loop_start = current_line();
        emit_instruction(OpCode::Lod, lexical_level, address, entry->identifier);
        if (!emit_expression(node->children[1])) {
            return;
        }
        emit_opr(downto ? OprCode::Geq : OprCode::Leq);
        const std::size_t jpc_end_line = emit_simple(OpCode::Jpc, 0, "for end");

        generate_statement(node->children[2]);

        emit_instruction(OpCode::Lod, lexical_level, address, entry->identifier);
        emit_literal(RuntimeValue::integer(1), "for step");
        emit_opr(downto ? OprCode::Sub : OprCode::Add);
        emit_instruction(OpCode::Sto, lexical_level, address, entry->identifier);
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
                return emit_user_call(node, true, false);
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
    const int parameter_slots = block.psze < 0 ? 0 : block.psze;
    const int variable_slots = block.vsze < 0 ? 0 : block.vsze;
    return kFrameHeaderSlots + static_cast<std::size_t>(parameter_slots + variable_slots);
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
