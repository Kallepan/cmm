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

    std::optional<node::Term*> parse_term() {
        if (auto int_lit = try_consume(TokenType::INT_LIT)) {
            auto term_int_lit = m_allocator.allocate<node::TermIntLit>();
            term_int_lit->int_lit = int_lit.value();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_int_lit;
            return term;
        }

        if (auto ident = try_consume(TokenType::IDENT)) {
            auto term_ident = m_allocator.allocate<node::TermIdent>();
            term_ident->ident = ident.value();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_ident;
            return term;
        }

        return std::nullopt;
    }

    std::optional<node::Expr*> parse_expr() {
        if (auto term = parse_term()) {
            if (try_consume(TokenType::PLUS)) {
                // Convert the term to an expression
                auto lhs_expr = m_allocator.allocate<node::Expr>();
                lhs_expr->var = term.value();

                auto rhs_expr = parse_expr();
                if (!rhs_expr.has_value()) {
                    std::cerr << "Syntax error: expected expression after +\n";
                    exit(EXIT_FAILURE);
                }

                auto bin_expr_add = m_allocator.allocate<node::BinExprAdd>();
                bin_expr_add->left = lhs_expr;
                bin_expr_add->right = rhs_expr.value();

                auto bin_expr = m_allocator.allocate<node::BinExpr>();
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
        if (try_consume(TokenType::EXIT, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
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

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");
            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* stmt = m_allocator.allocate<node::Stmt>();
            stmt->var = stmt_exit;
            return stmt;
        }

        // Parse let statement
        if (try_consume(TokenType::LET, false) &&
            try_consume(TokenType::IDENT, false, 1) &&
            try_consume(TokenType::EQ, false, 2)) {
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

            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

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

    inline Token try_consume(TokenType type, const std::string& error_message) {
        auto lexme = peek();
        if (lexme.has_value() && lexme.value().type == type) {
            return consume();
        }

        std::cerr << error_message << "\n";
        exit(EXIT_FAILURE);
    }

    inline std::optional<Token> try_consume(TokenType type,
                                            bool consume_token = true,
                                            size_t offset = 0) {
        auto lexme = peek(offset);
        if (lexme.has_value() && lexme.value().type == type) {
            if (consume_token) return consume();
            return lexme;
        }

        return std::nullopt;
    }

    const std::vector<Token> m_tokens;
    size_t m_index{0};
    ArenaAllocator m_allocator;
};