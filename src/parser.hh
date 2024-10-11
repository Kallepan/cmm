#pragma once

#include <utility>
#include <variant>
#include <vector>

#include "tokenization.hh"

namespace node {
struct ExprIntLit {
    Token int_lit;
};

struct ExprIdent {
    Token ident;
};

struct Expr {
    std::variant<ExprIntLit, ExprIdent> var;
};

struct StmtExit {
    Expr expr;
};

struct StmtLet {
    Token ident;
    Expr expr;
};

struct Stmt {
    std::variant<StmtExit, StmtLet> var;
};

struct Prog {
    std::vector<Stmt> stmts;
};
}  // namespace node

class Parser {
   public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {}

    std::optional<node::Expr> parse_expr() {
        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            return node::Expr{node::ExprIntLit{consume()}};
        }

        if (peek().has_value() && peek().value().type == TokenType::IDENT) {
            return node::Expr{node::ExprIdent{consume()}};
        }

        return std::nullopt;
    }

    std::optional<node::Stmt> parse_stmt() {
        // Parse exit statement
        if (peek().value().type == TokenType::EXIT && peek(1).has_value() &&
            peek(1).value().type == TokenType::OPEN_PAREN) {
            consume();
            consume();
            node::StmtExit stmt_exit;

            if (auto node_expr = parse_expr()) {
                stmt_exit = node::StmtExit{node_expr.value()};
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

            return node::Stmt{stmt_exit};
        }

        // Parse let statement
        if (peek().has_value() && peek().value().type == TokenType::LET &&
            peek(1).has_value() && peek(1).value().type == TokenType::IDENT &&
            peek(2).has_value() && peek(2).value().type == TokenType::EQ) {
            consume();
            auto stmt_let = node::StmtLet{consume(), node::Expr{}};
            consume();
            if (auto node_expr = parse_expr()) {
                stmt_let.expr = node_expr.value();
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

            return node::Stmt{stmt_let};
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
};