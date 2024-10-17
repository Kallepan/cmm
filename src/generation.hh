#pragma once

#include <algorithm>
#include <cassert>
#include <unordered_map>
#include <utility>

#include "parser.hh"

class Generator {
   public:
    inline explicit Generator(node::Prog prog) : m_prog(std::move(prog)) {}

    void gen_term(const node::Term* term) {
        struct TermVisitor {
            Generator* gen;
            void operator()(const node::TermIntLit* term_int_lit) const {
                gen->m_output << "    mov rax, " << term_int_lit->int_lit.value
                              << "\n";
                gen->push("rax");
            }

            void operator()(const node::TermIdent* term_ident) const {
                if (gen->m_vars.find(term_ident->ident.value) ==
                    gen->m_vars.end()) {
                    std::cerr
                        << "Variable not declared: " << term_ident->ident.value
                        << "\n";
                    exit(EXIT_FAILURE);
                }

                const auto& var = gen->m_vars.at(term_ident->ident.value);
                gen->push("QWORD [rsp + " +
                          std::to_string(
                              (gen->m_stack_pointer - var.stack_loc - 1) * 8) +
                          "]");
            }
        };

        std::visit(TermVisitor{this}, term->var);
    }

    void gen_bin_expr(const node::BinExpr* bin_expr) {
        struct BinExprVisitor {
            Generator* gen;

            void operator()(const node::BinExprAdd* bin_expr_add) const {
                gen->gen_expr(bin_expr_add->left);
                gen->gen_expr(bin_expr_add->right);

                gen->pop("rax");
                gen->pop("rbx");
                gen->m_output << "    add rax, rbx\n";
                gen->push("rax");
            }

            void operator()(const node::BinExprMultiply* bin_expr_mult) const {
                assert(false);  // TODO
            }
        };

        std::visit(BinExprVisitor{this}, bin_expr->var);
    }

    void gen_expr(const node::Expr* expr) {
        struct ExprVisitor {
            Generator* gen;

            void operator()(const node::Term* term) const {
                gen->gen_term(term);
            }

            void operator()(const node::BinExpr* bin_expr) const {
                gen->gen_bin_expr(bin_expr);
            }
        };

        std::visit(ExprVisitor{this}, expr->var);
    }

    void gen_stmt(const node::Stmt* stmt) {
        struct StmtVisitor {
            Generator* gen;

            void operator()(const node::StmtExit* stmt_exit) const {
                gen->gen_expr(stmt_exit->expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const node::StmtLet* stmt_let) const {
                if (gen->m_vars.contains(stmt_let->ident.value)) {
                    std::cerr << "Variable already declared: "
                              << stmt_let->ident.value << "\n";
                    exit(EXIT_FAILURE);
                }

                gen->m_vars.insert(
                    {stmt_let->ident.value, Var{gen->m_stack_pointer}});
                gen->gen_expr(stmt_let->expr);
            }
        };

        std::visit(StmtVisitor{this}, stmt->var);
    }

    [[nodiscard]] std::string gen_prog() {
        m_output << "global _start\n\n_start:\n";

        // Parse: start
        for (const auto& stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }
        // Parse: end

        //  Default exit
        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall\n";

        return m_output.str();
    }

   private:
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
        size_t stack_loc;
    };
    std::unordered_map<std::string, Var> m_vars{};
};