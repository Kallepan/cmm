#pragma once

#include <algorithm>
#include <unordered_map>
#include <utility>

#include "parser.hh"

class Generator {
   public:
    inline explicit Generator(node::Prog prog) : m_prog(std::move(prog)) {}

    void gen_expr(const node::Expr& expr) {
        struct ExprVisitor {
            Generator* gen;

            void operator()(const node::ExprIntLit& expr_int_lit) const {
                gen->m_output << "    mov rax, " << expr_int_lit.int_lit.value
                              << "\n";
                gen->push("rax");
            }

            void operator()(const node::ExprIdent& expr_ident) const {
                if (std::find_if(gen->m_vars.begin(), gen->m_vars.end(),
                                 [&expr_ident](const auto& var) {
                                     return var.first == expr_ident.ident.value;
                                 }) == gen->m_vars.end()) {
                    std::cerr
                        << "Variable not declared: " << expr_ident.ident.value
                        << "\n";
                    exit(EXIT_FAILURE);
                }

                const auto& var = gen->m_vars.at(expr_ident.ident.value);
                gen->push("QWORD [rsp + " +
                          std::to_string(
                              (gen->m_stack_pointer - var.stack_loc - 1) * 8) +
                          "]");
            }
        };

        std::visit(ExprVisitor{this}, expr.var);
    }

    void gen_stmt(const node::Stmt& stmt) {
        struct StmtVisitor {
            Generator* gen;

            void operator()(const node::StmtExit& stmt_exit) const {
                gen->gen_expr(stmt_exit.expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const node::StmtLet& stmt_let) const {
                if (gen->m_vars.contains(stmt_let.ident.value)) {
                    std::cerr
                        << "Variable already declared: " << stmt_let.ident.value
                        << "\n";
                    exit(EXIT_FAILURE);
                }

                gen->m_vars.insert(
                    {stmt_let.ident.value, Var{gen->m_stack_pointer}});
                gen->gen_expr(stmt_let.expr);
            }
        };

        std::visit(StmtVisitor{this}, stmt.var);
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