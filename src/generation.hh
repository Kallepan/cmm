#pragma once

#include <algorithm>
#include <cassert>
#include <sstream>
#include <utility>

#include "config.hh"
#include "parser.hh"

class Generator {
   public:
    explicit Generator(node::Prog prog) : m_prog(std::move(prog)) {}

    void gen_term(const node::Term* term) {
        struct TermVisitor {
            Generator& gen;
            void operator()(
                const node::TermIntLit* term_integer_literal) const {
                gen.m_start << "    mov rax, "
                            << term_integer_literal->integer_literal.value
                            << "\n";
                gen.push("rax");
            }

            void operator()(const node::TermIdent* term_identifier) const {
                // Check if the variable is declared by looking it up in the
                // variable vector using find_if
                const auto iterator = std::find_if(
                    gen.m_vars.cbegin(), gen.m_vars.cend(),
                    [&](const Var& var) {
                        return var.name == term_identifier->identifier.value;
                    });
                if (iterator == gen.m_vars.cend()) {
                    std::cerr << "Variable not declared: "
                              << term_identifier->identifier.value << "\n";
                    exit(EXIT_FAILURE);
                }

                std::ostringstream oss;
                oss << "QWORD [rsp + "
                    << (gen.m_stack_pointer - iterator->stack_loc - 1) * 8
                    << "]";
                gen.push(oss.str());
            }

            void operator()(const node::TermParen* term_parenthesis) const {
                gen.gen_expr(term_parenthesis->expression);
            }
        };

        std::visit(TermVisitor{*this}, term->var);
    }

    void gen_bin_expr(const node::BinExpr* bin_expr) {
        struct BinExprVisitor {
            Generator& gen;

            void operator()(
                const node::BinExprAddition* expr_binary_addition) const {
                gen.gen_expr(expr_binary_addition->right);
                gen.gen_expr(expr_binary_addition->left);

                gen.pop("rax");
                gen.pop("rbx");
                gen.m_start << "    add rax, rbx\n";
                gen.push("rax");
            }

            void operator()(
                const node::BinExprSubtraction* expr_binary_sub) const {
                gen.gen_expr(expr_binary_sub->right);
                gen.gen_expr(expr_binary_sub->left);

                gen.pop("rax");
                gen.pop("rbx");
                gen.m_start << "    sub rax, rbx\n";
                gen.push("rax");
            }

            void operator()(
                const node::BinExprMultiplication* expr_binary_multiply) const {
                gen.gen_expr(expr_binary_multiply->left);
                gen.gen_expr(expr_binary_multiply->right);

                gen.pop("rax");
                gen.pop("rbx");
                gen.m_start << "    xor rdx, rdx\n";  // Clear the high bits
                gen.m_start << "    mul rbx\n";
                gen.push("rax");
            }

            void operator()(
                const node::BinExprDivision* expr_binary_div) const {
                gen.gen_expr(expr_binary_div->left);
                gen.gen_expr(expr_binary_div->right);

                gen.pop("rbx");
                gen.pop("rax");
                gen.m_start << "    cqo\n";
                gen.m_start << "    idiv rbx\n";
                gen.push("rax");
            }
        };

        std::visit(BinExprVisitor{*this}, bin_expr->var);
    }

    void gen_scope(const node::Scope* scope) {
        begin_scope();

        for (const auto& statement : scope->statements) {
            gen_stmt(statement);
        }

        end_scope();
    }

    void gen_if_predicate(const node::IfPred* pred,
                          const std::string& end_jump_label) {
        struct PredicateVisitor {
            Generator& gen;
            const std::string& end_jump_label;

            void operator()(const node::IfPredElif* pred_elif) const {
                gen.gen_expr(pred_elif->condition);
                gen.pop("rax");
                const std::string label = gen.create_label();

                gen.m_start << "    test rax, rax\n";
                gen.m_start << "    jz " << label << "\n";

                gen.gen_scope(pred_elif->scope);
                gen.m_start << "    jmp " << end_jump_label << "\n";

                gen.m_start << label << ":\n";
                if (pred_elif->next.has_value()) {
                    gen.gen_if_predicate(pred_elif->next.value(),
                                         end_jump_label);
                }
            }

            void operator()(const node::IfPredElse* pred_else) const {
                gen.gen_scope(pred_else->scope);
            }
        };

        std::visit(PredicateVisitor{*this, end_jump_label}, pred->var);
    }

    void gen_string_literal(const std::string string_literal) {
        size_t current_string_counter = m_string_counter++;

        // Add string to the data section
        m_data << "    string" << current_string_counter << " db '";

        for (const char c : string_literal) {
            if (c == '\n') {
                m_data << "', 10, '";
                continue;
            }

            m_data << c;
        }
        m_data << "', 0\n";

        // Add string length + 1 (for the null terminator) to the data section
        m_data << "    string" << current_string_counter << "_len"
               << " equ " << string_literal.size() + 1 << "\n";

        // Load the address of the string into rsi
        m_start << "    lea rsi, [string" << current_string_counter << "]\n";
        // Load the length of the string into rcx
        m_start << "    mov rcx, string" << current_string_counter << "_len"
                << "\n";
        m_start << "    call check_and_add_to_buffer\n";
    }

    void gen_expr(const node::Expr* expression) {
        struct ExprVisitor {
            Generator& gen;

            void operator()(const node::Term* term) const {
                gen.gen_term(term);
            }

            void operator()(const node::BinExpr* bin_expr) const {
                gen.gen_bin_expr(bin_expr);
            }
        };

        std::visit(ExprVisitor{*this}, expression->var);
    }

    void gen_arg(const node::StmtArg* statement_print) {
        struct ArgVisitor {
            Generator& gen;

            void operator()(const node::Expr* expression) const {
                gen.gen_expr(expression);
                gen.m_start << "    mov rsi, QWORD [rsp]\n";
                gen.m_start << "    call print_int\n";
                gen.m_start << "    call print_newline\n";
            }

            void operator()(const node::StringLit* string_literal) const {
                gen.gen_string_literal(string_literal->string_literal.value);
            }
        };

        std::visit(ArgVisitor{*this}, statement_print->var);
    }

    void gen_stmt(const node::Stmt* statement) {
        struct StmtVisitor {
            Generator& gen;

            void operator()(const node::StmtExit* statement_exit) const {
                gen.gen_expr(statement_exit->expression);

                gen.m_start << "    call flush_buffer\n";
                gen.m_start << "    mov rax, 60\n";
                gen.pop("rdi");
                gen.m_start << "    syscall\n\n";
            }

            void operator()(const node::StmtArg* statement_print) const {
                gen.gen_arg(statement_print);
            }

            void operator()(const node::StmtLet* statement_let) const {
                // Check if the variable is already declared in the current
                // scope
                const auto iterator = std::find_if(
                    gen.m_vars.cbegin(), gen.m_vars.cend(),
                    [&](const Var& var) {
                        return var.name == statement_let->identifier.value &&
                               var.scope == gen.m_stack_scopes.size() - 1;
                    });
                if (iterator != gen.m_vars.cend()) {
                    std::cerr << "Variable already declared: "
                              << statement_let->identifier.value << "\n";
                    exit(EXIT_FAILURE);
                }

                gen.m_vars.push_back(Var{
                    statement_let->identifier.value, statement_let->is_mutable,
                    gen.m_stack_pointer, gen.m_stack_scopes.size() - 1});
                gen.gen_expr(statement_let->expression);
            }

            void operator()(const node::Scope* scope) const {
                gen.gen_scope(scope);
            }

            void operator()(const node::StmtIf* statement_if) const {
                gen.gen_expr(statement_if->condition);
                gen.pop("rax");
                const std::string label = gen.create_label();

                gen.m_start << "    test rax, rax\n";
                gen.m_start << "    jz " << label << "\n";

                gen.gen_scope(statement_if->scope);
                const std::string end_label = gen.create_label();
                gen.m_start << "    jmp " << end_label << "\n";

                gen.m_start << label << ":\n";
                if (statement_if->next.has_value()) {
                    gen.gen_if_predicate(statement_if->next.value(), end_label);
                }

                gen.m_start << end_label << ":\n";
            }
        };

        std::visit(StmtVisitor{*this}, statement->var);
    }

    [[nodiscard]] std::string gen_prog() {
        m_start << "section .text\n"
                << "    global _start\n\n_start:\n"
                << "    call initialize_buffer\n";

        m_data << "section .data\n"
               << "    newline db 10\n";
        std::ostringstream buffer;
        buffer << "section .bss\n"
               << "    buffer resb " << m_buffer_size << "\n"
               << "    buffer_used resq 1\n\n"
               << "    buffer_size equ " << m_buffer_size << "\n\n";

        // Functions
        std::string functions =
            "initialize_buffer:\n"
            "    mov qword [buffer_used], 0\n"  // Reset buffer_used
            "\ncheck_and_add_to_buffer:\n"
            "    mov rax, [buffer_used]\n"  // rax = buffer_used
            "    add rax, rcx\n"            // rax = buffer_used + string_length
            "    cmp rax, buffer_size\n"  // Compare buffer_used + string_length
                                          // with buffer_size
            "    jle add_to_buffer\n"     // If buffer_used + string_length <=
                                          // buffer_size, add string to buffer
            "    call flush_buffer\n"     // If buffer_used + string_length >
                                          // buffer_size, flush buffer
            "    call initialize_buffer\n"  // Reset buffer_used
            "    jmp add_to_buffer\n"       // Add string to buffer
            "\nadd_to_buffer:\n"
            "    mov rax, [buffer_used]\n"        // rax = buffer_used
            "    lea rdi, [buffer + rax]\n"       // rdi = buffer + buffer_used
            "    add qword [buffer_used], rcx\n"  // buffer_used +=
                                                  // string_length
            "    rep movsb\n"                     // Copy string to buffer
            "    ret\n"
            "\nflush_buffer:\n"
            "    lea rsi, [buffer]\n"       // rsi = buffer
            "    mov rdx, [buffer_used]\n"  // rdx = buffer_used
            "    call print_chars\n"
            "    call print_newline\n"
            "    ret\n"
            "print_newline:\n"
            "    mov rsi, newline\n"  // rsi = newline
            "    mov rdx, 1\n"        // rdx = 1
            "    call print_chars\n"
            "    ret\n"
            "print_chars:\n"
            "    mov rdi, 1\n"  // rdi = stdout
            "    mov rax, 1\n"  // rax = sys_write
            "    syscall\n"
            "    ret\n"
            "print_int_h:\n"
            "    push rax\n"      // Save rax
            "    push rbp\n"      // Save rbp
            "    push rsi\n"      // Save rsi
            "    push rdx\n"      // Save rdx
            "    mov rbp, rsp\n"  // Save base pointer
            ".loop:\n"
            "    mov al, sil\n"        // Load the least significant digit
            "    and al, 0x0F\n"       // Mask to get the last hex digit
            "    cmp al, 9\n"          // Check if al > 9
            "    jle .insert_digit\n"  // If al <= 9, insert digit
            "    add al, 87\n"         // Convert to ASCII a-f (97 - 10)
            "    jmp .insert_byte\n"
            ".insert_digit:\n"
            "    add al, 48\n"  // Convert to ASCII 0-9
            ".insert_byte:\n"
            "    dec rsp\n"  // Move the stack pointer
            "    mov [rsp], al\n"
            "    shr rsi, 4\n"  // Shift right 4 bits
            "    test rsi, rsi\n"
            "    jnz .loop\n"
            "    dec rsp\n"              // Move the stack pointer
            "    mov [rsp], byte 120\n"  // Insert x
            "    dec rsp\n"              // Move the stack pointer
            "    mov [rsp], byte 48\n"   // Insert 0
            "    mov rdx, rbp\n"         // rdx = rsp
            "    sub rdx, rsp\n"         // rdx = rsp - rbp
            "    lea rsi, [rsp]\n"       // rsi = rsp
            "    mov rdx, rdx\n"         // rdx = rdx
            "    call print_chars\n"
            "    mov rsp, rbp\n"  // Restore stack pointer
            "    pop rdx\n"       // Restore rdx
            "    pop rsi\n"       // Restore rsi
            "    pop rbp\n"       // Restore rbp
            "    pop rax\n"       // Restore rax
            "    ret\n"
            "print_int:\n"
            "    push rax\n"       // Save rax
            "    push rbp\n"       // Save rbp
            "    push rsi\n"       // Save rsi
            "    push rdx\n"       // Save rdx
            "    push r8\n"        // Save r8
            "    mov r8, rsi\n"    // move original rsi to r8
            "    mov rax, rsi\n"   // rax = rsi
            "    test rax, rax\n"  // Check if rsi is negative
            "    jns .positive\n"  // If rsi is positive, jump to .positive
            "    neg rax\n"        // Negate rsi
            ".positive:\n"
            "    mov rsi, 10\n"   // Clear rsi
            "    mov rbp, rsp\n"  // Save base pointer
            ".loop:\n"
            "    xor rdx, rdx\n"      // Clear rdx
            "    div rsi\n"           // Divide rax by rsi
            "    add dl, 48\n"        // Convert to ASCII
            "    dec rsp\n"           // Move the stack pointer
            "    mov [rsp], dl\n"     // Insert digit
            "    test rax, rax\n"     // Check if rax is zero
            "    jnz .loop\n"         // If rax is not zero, jump to .loop
            "    test r8, r8\n"       // Check if r8 is negative
            "    jns .no_neg_sign\n"  // If r8 is positive, jump to .no_neg_sign
            "    dec rsp\n"           // Move the stack pointer
            "    mov [rsp], byte 45\n"  // Insert -
            ".no_neg_sign:\n"
            "    mov rdx, rbp\n"  // rdx = rsp
            "    sub rdx, rsp\n"  // rdx = rsp - rbp
            "    mov rsi, rsp\n"  // rsi = rsp
            "    mov rdx, rdx\n"  // rdx = rdx
            "    call print_chars\n"
            "    mov rsp, rbp\n"  // Restore stack pointer
            "    pop r8\n"        // Restore r8
            "    pop rdx\n"       // Restore rdx
            "    pop rsi\n"       // Restore rsi
            "    pop rbp\n"       // Restore rbp
            "    pop rax\n"       // Restore rax
            "    ret\n";

        "\n";

        // Parse: start
        for (const auto& statement : m_prog.statements) {
            gen_stmt(statement);
        }
        // Parse: end

        //  Default exit
        if (m_prog.statements.empty() ||
            !std::holds_alternative<node::StmtExit*>(
                m_prog.statements.back()->var)) {
            m_start << "    call print_chars\n";
            m_start << "    mov rdi, 0\n";
            m_start << "    mov rax, 60\n";
            m_start << "    syscall\n\n";
        }

        return m_data.str() + buffer.str() + m_start.str() + functions;
    }

   private:
    void begin_scope() { m_stack_scopes.push_back(m_vars.size()); }
    void end_scope() {
        const size_t variables_to_pop = m_vars.size() - m_stack_scopes.back();
        if (variables_to_pop == 0) {
            return;
        }
        m_start << "    add rsp, " << variables_to_pop * 8 << "\n";
        m_stack_pointer -= variables_to_pop;
        m_vars.resize(m_stack_scopes.back());
        m_stack_scopes.pop_back();
    }

    std::string create_label() {
        std::ostringstream oss;
        oss << ".L" << m_label_counter++;
        return oss.str();
    }

    void push(const std::string& reg) {
        m_start << "    push " << reg << "\n";
        m_stack_pointer++;
    }

    void pop(const std::string& reg) {
        m_start << "    pop " << reg << "\n";
        m_stack_pointer--;
    }

    const node::Prog m_prog;
    std::ostringstream m_start;
    std::ostringstream m_data;

    // Keeps track of the stack pointer
    size_t m_stack_pointer = 0;

    // Keeps track of the variable names
    struct Var {
        std::string name;
        bool is_mutable;
        size_t stack_loc;
        size_t scope;
    };
    std::vector<Var> m_vars;             // Keeps track of the variables
    std::vector<size_t> m_stack_scopes;  // Keeps track of the stack scopes
    size_t m_label_counter = 0;          // Keeps track of the number of labels
    size_t m_string_counter = 0;         // Keeps track of the number of strings
    const size_t m_buffer_size = PRINT_BUFFER_SIZE;  // Buffer size
};