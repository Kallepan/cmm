#pragma once

#include <utility>
#include <variant>
#include <vector>

#include "arena.hh"
#include "tokenization.hh"

namespace node {
struct TermIntLit {
    Token integer_literal;
};

struct TermIdent {
    Token identifier;
};

struct Expr;

struct BinExprAddition {
    Expr* left;
    Expr* right;
};

struct BinExprMultiply {
    Expr* left;
    Expr* right;
};

struct BinExpr {
    std::variant<BinExprAddition*, BinExprMultiply*> var;
};

struct Term {
    std::variant<TermIntLit*, TermIdent*> var;
};

struct Expr {
    std::variant<Term*, BinExpr*> var;
};

struct StmtExit {
    Expr* expression;
};

struct StmtLet {
    Token identifier;
    Expr* expression;
};

struct Stmt {
    std::variant<StmtExit*, StmtLet*> var;
};

struct Prog {
    std::vector<Stmt*> statements;
};
}  // namespace node

class Parser {
   public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)),
          m_allocator(1024 * 1024 * 4)  // 4 mb
    {}

    std::optional<node::Term*> parse_term() {
        if (auto integer_literal = try_consume(TokenType::INT_LIT)) {
            auto term_int_lit = m_allocator.allocate<node::TermIntLit>();
            term_int_lit->integer_literal = integer_literal.value();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_int_lit;
            return term;
        }

        if (auto identifier = try_consume(TokenType::IDENT)) {
            auto term_ident = m_allocator.allocate<node::TermIdent>();
            term_ident->identifier = identifier.value();

            auto term = m_allocator.allocate<node::Term>();
            term->var = term_ident;
            return term;
        }

        return std::nullopt;
    }

    // Function takes the minimum precedence level (default 0) and returns the
    // corresponding expression
    std::optional<node::Expr*> parse_expr(int minimum_precedence = 0) {
        std::optional<node::Term*> term_lhs = parse_term();
        if (!term_lhs.has_value()) {
            return std::nullopt;
        }

        auto expression = m_allocator.allocate<node::Expr>();
        expression->var = term_lhs.value();

        while (true) {
            std::optional<Token> next_token = peek();
            if (!next_token.has_value()) {
                break;
            }

            std::optional<int> precedence = bin_prec(next_token->type);
            if (!precedence.has_value() ||
                precedence.value() < minimum_precedence) {
                break;
            }

            Token operator_token = consume();
            int next_minimum_precedence = precedence.value() + 1;
            auto expr_rhs = parse_expr(next_minimum_precedence);
            if (!expr_rhs.has_value()) {
                std::cerr
                    << "Syntax error: expected expression after operator\n";
                exit(EXIT_FAILURE);
            }

            auto bin_expr = m_allocator.allocate<node::BinExpr>();
            auto expr_lhs = m_allocator.allocate<node::Expr>();
            if (operator_token.type == TokenType::PLUS) {
                expr_lhs->var = expression->var;

                auto expr_binary_addition =
                    m_allocator.allocate<node::BinExprAddition>();
                expr_binary_addition->left = expr_lhs;
                expr_binary_addition->right = expr_rhs.value();

                bin_expr->var = expr_binary_addition;
            } else if (operator_token.type == TokenType::MULTIPLY) {
                expr_lhs->var = expression->var;

                auto expr_binary_multiply =
                    m_allocator.allocate<node::BinExprMultiply>();
                expr_binary_multiply->left = expr_lhs;
                expr_binary_multiply->right = expr_rhs.value();

                bin_expr->var = expr_binary_multiply;
            }

            expression->var = bin_expr;
        }

        return expression;
    }

    std::optional<node::Stmt*> parse_stmt() {
        // Parse exit statement
        if (try_consume(TokenType::EXIT, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtExit* stmt_exit = m_allocator.allocate<node::StmtExit>();

            if (auto node_expr = parse_expr()) {
                stmt_exit->expression = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected integer literal after exit"
                          << "\n";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");
            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* statement = m_allocator.allocate<node::Stmt>();
            statement->var = stmt_exit;
            return statement;
        }

        // Parse let statement
        if (try_consume(TokenType::LET, false) &&
            try_consume(TokenType::IDENT, false, 1) &&
            try_consume(TokenType::EQ, false, 2)) {
            consume();
            node::StmtLet* statement_let = m_allocator.allocate<node::StmtLet>();
            statement_let->identifier = consume();
            consume();
            if (auto node_expr = parse_expr()) {
                statement_let->expression = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected integer literal after let"
                          << "\n";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* statement = m_allocator.allocate<node::Stmt>();
            statement->var = statement_let;
            return statement;
        }

        return std::nullopt;
    }

    std::optional<node::Prog> parse_prog() {
        node::Prog prog{};
        while (peek().has_value()) {
            if (auto statement = parse_stmt()) {
                prog.statements.push_back(statement.value());
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