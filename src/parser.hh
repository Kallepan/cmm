#pragma once

#include <utility>
#include <variant>
#include <vector>

#include "arena.hh"
#include "tokenization.hh"

namespace node {
struct TermIntLit {
    Token int_lit;
};

struct TermIdent {
    Token ident;
};

struct Expr;

struct BinExprAdd {
    Expr* left;
    Expr* right;
};

struct BinExprMultiply {
    Expr* left;
    Expr* right;
};

struct BinExpr {
    std::variant<BinExprAdd*, BinExprMultiply*> var;
};

struct Term {
    std::variant<TermIntLit*, TermIdent*> var;
};

struct Expr {
    std::variant<Term*, BinExpr*> var;
};

struct StmtExit {
    Expr* expr;
};

struct StmtLet {
    Token ident;
    Expr* expr;
};

struct Stmt {
    std::variant<StmtExit*, StmtLet*> var;
};

struct Prog {
    std::vector<Stmt*> stmts;
};
}  // namespace node

class Parser {
   public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
          m_allocator(1024 * 1024 * 4)  // 4 mb
    {}

    std::optional<node::BinExpr*> parse_bin_expr() {
        if (auto left_expr = parse_expr()) {
            auto bin_expr = m_allocator.allocate<node::BinExpr>();

            auto right_expr = parse_expr();
            if (!right_expr.has_value()) {
                std::cerr << "Syntax error: expected expression after +\n";
                exit(EXIT_FAILURE);
            }

            auto bin_expr_add = m_allocator.allocate<node::BinExprAdd>();
            bin_expr_add->left = left_expr.value();
            bin_expr_add->right = right_expr.value();

            bin_expr->var = bin_expr_add;
            return bin_expr;

            std::cerr << "Unsupported binary operator\n";
        }

        return std::nullopt;
    }

    std::optional<node::Term*> parse_term() {
        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            auto term_int_lit = m_allocator.allocate<node::TermIntLit>();
            term_int_lit->int_lit = consume();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_int_lit;
            return term;
        }

        if (peek().has_value() && peek().value().type == TokenType::IDENT) {
            auto term_ident = m_allocator.allocate<node::TermIdent>();
            term_ident->ident = consume();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_ident;
            return term;
        }

        return std::nullopt;
    }

    std::optional<node::Expr*> parse_expr() {
        if (auto term = parse_term()) {
            if (peek().has_value() && peek().value().type == TokenType::PLUS) {
                consume();
                auto bin_expr = m_allocator.allocate<node::BinExpr>();
                auto bin_expr_add = m_allocator.allocate<node::BinExprAdd>();
                // Convert the term to an expression
                auto lhs_expr = m_allocator.allocate<node::Expr>();
                lhs_expr->var = term.value();

                auto right_expr = parse_expr();
                if (!right_expr.has_value()) {
                    std::cerr << "Syntax error: expected expression after +\n";
                    exit(EXIT_FAILURE);
                }

                bin_expr_add->right = right_expr.value();
                bin_expr->var = bin_expr_add;

                auto expr = m_allocator.allocate<node::Expr>();
                expr->var = bin_expr;
                return expr;
            }

            auto expr = m_allocator.allocate<node::Expr>();
            expr->var = term.value();
            return expr;
        }

        return std::nullopt;
    }

    std::optional<node::Stmt*> parse_stmt() {
        // Parse exit statement
        if (peek().value().type == TokenType::EXIT && peek(1).has_value() &&
            peek(1).value().type == TokenType::OPEN_PAREN) {
            consume();
            consume();
            node::StmtExit* stmt_exit = m_allocator.allocate<node::StmtExit>();

            if (auto node_expr = parse_expr()) {
                stmt_exit->expr = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected integer literal after exit"
                          << "\n";
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() &&
                peek().value().type == TokenType::CLOSE_PAREN) {
                consume();
            } else {
                std::cerr << "Syntax error: expected )\n";
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() ||
                peek().value().type == TokenType::END_OF_LINE) {
                consume();
            } else {
                std::cerr << "Syntax error: expected ;\n";
                exit(EXIT_FAILURE);
            }

            node::Stmt* stmt = m_allocator.allocate<node::Stmt>();
            stmt->var = stmt_exit;
            return stmt;
        }

        // Parse let statement
        if (peek().has_value() && peek().value().type == TokenType::LET &&
            peek(1).has_value() && peek(1).value().type == TokenType::IDENT &&
            peek(2).has_value() && peek(2).value().type == TokenType::EQ) {
            consume();
            node::StmtLet* stmt_let = m_allocator.allocate<node::StmtLet>();
            stmt_let->ident = consume();
            consume();
            if (auto node_expr = parse_expr()) {
                stmt_let->expr = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected integer literal after let"
                          << "\n";
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() ||
                peek().value().type == TokenType::END_OF_LINE) {
                consume();
            } else {
                std::cerr << "Syntax error: expected ;\n";
                exit(EXIT_FAILURE);
            }

            node::Stmt* stmt = m_allocator.allocate<node::Stmt>();
            stmt->var = stmt_let;
            return stmt;
        }

        return std::nullopt;
    }

    std::optional<node::Prog> parse_prog() {
        node::Prog prog{};
        while (peek().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            } else {
                std::cerr << "Syntax error: expected statement\n";
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

   private:
    [[nodiscard]] inline std::optional<Token> peek(size_t offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return std::nullopt;
        }

        return m_tokens.at(m_index + offset);
    }

    inline Token consume() { return m_tokens.at(m_index++); }

    const std::vector<Token> m_tokens;
    size_t m_index{0};
    ArenaAllocator m_allocator;
};