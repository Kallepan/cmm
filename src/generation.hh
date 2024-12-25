#pragma once

#include <algorithm>
#include <cassert>
#include <utility>

#include "parser.hh"

class Generator {
   public:
    inline explicit Generator(node::Prog prog) : m_prog(std::move(prog)) {}

    void gen_term(const node::Term* term) {
        struct TermVisitor {
            Generator* gen;
            void operator()(
                const node::TermIntLit* term_integer_literal) const {
                gen->m_output << "    mov rax, "
                              << term_integer_literal->integer_literal.value
                              << "\n";
                gen->push("rax");
            }

            void operator()(const node::TermIdent* term_identifier) const {
                // Check if the variable is declared by looking it up in the
                // variable vector using find_if
                auto idx = std::find_if(
                    gen->m_vars.cbegin(), gen->m_vars.cend(),
                    [&](const Var& var) {
                        return var.name == term_identifier->identifier.value;
                    });
                if (idx == gen->m_vars.cend()) {
                    std::cerr << "Variable not declared: "
                              << term_identifier->identifier.value << "\n";
                    exit(EXIT_FAILURE);
                }

                gen->push(
                    "QWORD [rsp + " +
                    std::to_string(
                        (gen->m_stack_pointer - (*idx).stack_loc - 1) * 8) +
                    "]");
            }

            void operator()(const node::TermParen* term_parenthesis) const {
                gen->gen_expr(term_parenthesis->expression);
            }
        };

        std::visit(TermVisitor{this}, term->var);
    }

    void gen_bin_expr(const node::BinExpr* bin_expr) {
        struct BinExprVisitor {
            Generator* gen;

            void operator()(
                const node::BinExprAddition* expr_binary_addition) const {
                gen->gen_expr(expr_binary_addition->right);
                gen->gen_expr(expr_binary_addition->left);

                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax");
            }

            void operator()(
                const node::BinExprSubtraction* expr_binary_sub) const {
                gen->gen_expr(expr_binary_sub->right);
                gen->gen_expr(expr_binary_sub->left);

                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    sub rax, rbx\n";
                gen->push("rax");
            }

            void operator()(
                const node::BinExprMultiplication* expr_binary_multiply) const {
                gen->gen_expr(expr_binary_multiply->left);
                gen->gen_expr(expr_binary_multiply->right);

                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    xor rdx, rdx\n";  // Clear the high bits
                gen->m_output << "    mul rbx\n";
                gen->push("rax");
            }

            void operator()(
                const node::BinExprDivision* expr_binary_div) const {
                gen->gen_expr(expr_binary_div->left);
                gen->gen_expr(expr_binary_div->right);

                gen->pop("rbx");
                gen->pop("rax");
                gen->m_output << "    cqo\n";
                gen->m_output << "    idiv rbx\n";
                gen->push("rax");
            }
        };

        std::visit(BinExprVisitor{this}, bin_expr->var);
    }

    void gen_expr(const node::Expr* expression) {
        struct ExprVisitor {
            Generator* gen;

            void operator()(const node::Term* term) const {
                gen->gen_term(term);
            }

            void operator()(const node::BinExpr* bin_expr) const {
                gen->gen_bin_expr(bin_expr);
            }
        };

        std::visit(ExprVisitor{this}, expression->var);
    }

    void gen_stmt(const node::Stmt* statement) {
        struct StmtVisitor {
            Generator* gen;

            void operator()(const node::StmtExit* stmt_exit) const {
                gen->gen_expr(stmt_exit->expression);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const node::StmtLet* statement_let) const {
                // Check if the variable is already declared in the current
                // scope
                auto iterator = std::find_if(
                    gen->m_vars.cbegin(), gen->m_vars.cend(),
                    [&](const Var& var) {
                        return var.name == statement_let->identifier.value &&
                               var.scope == gen->m_stack_scopes.size() - 1;
                    });
                if (iterator != gen->m_vars.cend()) {
                    std::cerr << "Variable already declared: "
                              << statement_let->identifier.value << "\n";
                    exit(EXIT_FAILURE);
                }

                gen->m_vars.push_back(Var{statement_let->identifier.value,
                                          gen->m_stack_pointer,
                                          gen->m_stack_scopes.size() - 1});
                gen->gen_expr(statement_let->expression);
            }

            void operator()(const node::StmtScope* scope) const {
                gen->begin_scope();

                for (const auto& statement : scope->statements) {
                    gen->gen_stmt(statement);
                }

                gen->end_scope();
            }
        };

        std::visit(StmtVisitor{this}, statement->var);
    }

    [[nodiscard]] std::string gen_prog() {
        m_output << "global _start\n\n_start:\n";

        // Parse: start
        for (const auto& statement : m_prog.statements) {
            gen_stmt(statement);
        }
        // Parse: end

        //  Default exit
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";

        return m_output.str();
    }

   private:
    void begin_scope() { m_stack_scopes.push_back(m_vars.size()); }
    void end_scope() {
        size_t variables_to_pop = m_vars.size() - m_stack_scopes.back();
        m_output << "    add rsp, " << variables_to_pop * 8 << "\n";
        m_stack_pointer -= variables_to_pop;
        m_vars.resize(m_stack_scopes.back());
        m_stack_scopes.pop_back();
    }

    void push(const std::string& reg) {
        m_output << "    push " << reg << "\n";
        m_stack_pointer++;
    }

    void pop(const std::string& reg) {
        m_output << "    pop " << reg << "\n";
        m_stack_pointer--;
    }

    const node::Prog m_prog;
    std::stringstream m_output;

    // Keeps track of the stack pointer
    size_t m_stack_pointer = 0;

    // Keeps track of the variable names
    struct Var {
        std::string name;
        size_t stack_loc;
        size_t scope;
    };
    std::vector<Var> m_vars;             // Keeps track of the variables
    std::vector<size_t> m_stack_scopes;  // Keeps track of the stack scopes
};