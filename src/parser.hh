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

struct TermParen {
    Expr* expression;
};

struct StringLit {
    Token string_literal;
};

struct BinExprAddition {
    Expr* left;
    Expr* right;
};

struct BinExprMultiplication {
    Expr* left;
    Expr* right;
};

struct BinExprSubtraction {
    Expr* left;
    Expr* right;
};

struct BinExprDivision {
    Expr* left;
    Expr* right;
};

struct BinExpr {
    std::variant<BinExprAddition*, BinExprMultiplication*, BinExprSubtraction*,
                 BinExprDivision*>
        var;
};

struct Term {
    std::variant<TermIntLit*, TermIdent*, TermParen*> var;
};

struct Expr {
    std::variant<Term*, BinExpr*> var;
};

struct StmtExit {
    Expr* expression;
};

struct StmtPrint {
    std::variant<Expr*, StringLit*> var;
};

struct StmtLet {
    Token identifier;
    Expr* expression;
};

struct Stmt;

struct Scope {
    std::vector<Stmt*> statements;
};

struct StmtIf {
    Expr* condition;
    Scope* scope;
};

struct Stmt {
    std::variant<StmtExit*, StmtPrint*, StmtLet*, Scope*, StmtIf*> var;
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
            auto term_int_lit = m_allocator.alloc<node::TermIntLit>();
            term_int_lit->integer_literal = integer_literal.value();

            auto term = m_allocator.alloc<node::Term>();
            term->var = term_int_lit;
            return term;
        }

        if (auto identifier = try_consume(TokenType::IDENT)) {
            auto term_ident = m_allocator.alloc<node::TermIdent>();
            term_ident->identifier = identifier.value();

            auto term = m_allocator.alloc<node::Term>();
            term->var = term_ident;
            return term;
        }

        if (auto open_paren = try_consume(TokenType::OPEN_PAREN)) {
            auto term_paren = m_allocator.alloc<node::TermParen>();
            auto expr = parse_expr();

            if (!expr.has_value()) {
                std::cerr << "Syntax error: expected expression after (\n";
                exit(EXIT_FAILURE);
            }

            term_paren->expression = expr.value();

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");

            auto term = m_allocator.alloc<node::Term>();
            term->var = term_paren;
            return term;
        }

        return std::nullopt;
    }

    std::optional<node::StringLit*> parse_string_lit() {
        if (auto string_literal = try_consume(TokenType::STRING_LIT)) {
            auto string_lit = m_allocator.alloc<node::StringLit>();
            string_lit->string_literal = string_literal.value();
            return string_lit;
        }

        return std::nullopt;
    }

    // Function takes the minimum precedence level (default 0) and returns the
    // corresponding expression
    std::optional<node::Expr*> parse_expr(const size_t minimum_precedence = 0) {
        std::optional<node::Term*> term_lhs = parse_term();
        if (!term_lhs.has_value()) {
            return std::nullopt;
        }

        auto expression = m_allocator.alloc<node::Expr>();
        expression->var = term_lhs.value();

        while (true) {
            std::optional<Token> next_token = peek();
            if (!next_token.has_value()) {
                break;
            }

            std::optional<size_t> precedence = bin_prec(next_token->type);
            if (!precedence.has_value() ||
                precedence.value() < minimum_precedence) {
                break;
            }

            const Token operator_token = consume();
            const size_t next_minimum_precedence = precedence.value() + 1;
            auto expr_rhs = parse_expr(next_minimum_precedence);
            if (!expr_rhs.has_value()) {
                std::cerr
                    << "Syntax error: expected expression after operator\n";
                exit(EXIT_FAILURE);
            }

            auto bin_expr = m_allocator.alloc<node::BinExpr>();
            auto expr_lhs = m_allocator.alloc<node::Expr>();
            if (operator_token.type == TokenType::PLUS) {
                expr_lhs->var = expression->var;

                auto expr_binary_addition =
                    m_allocator.alloc<node::BinExprAddition>();
                expr_binary_addition->left = expr_lhs;
                expr_binary_addition->right = expr_rhs.value();

                bin_expr->var = expr_binary_addition;
            } else if (operator_token.type == TokenType::MINUS) {
                expr_lhs->var = expression->var;

                auto expr_binary_subtraction =
                    m_allocator.alloc<node::BinExprSubtraction>();
                expr_binary_subtraction->left = expr_lhs;
                expr_binary_subtraction->right = expr_rhs.value();

                bin_expr->var = expr_binary_subtraction;
            } else if (operator_token.type == TokenType::FORWARD_SLASH) {
                expr_lhs->var = expression->var;

                auto expr_binary_division =
                    m_allocator.alloc<node::BinExprDivision>();
                expr_binary_division->left = expr_lhs;
                expr_binary_division->right = expr_rhs.value();

                bin_expr->var = expr_binary_division;
            } else if (operator_token.type == TokenType::STAR) {
                expr_lhs->var = expression->var;

                auto expr_binary_multiplication =
                    m_allocator.alloc<node::BinExprMultiplication>();
                expr_binary_multiplication->left = expr_lhs;
                expr_binary_multiplication->right = expr_rhs.value();

                bin_expr->var = expr_binary_multiplication;
            } else {
                std::cerr << "Syntax error: unknown operator\n";
                exit(EXIT_FAILURE);
            }

            expression->var = bin_expr;
        }

        return expression;
    }

    std::optional<node::Scope*> parse_scope() {
        if (try_consume(TokenType::OPEN_CURLY, "Syntax error: expected {\n")) {
            node::Scope* scope = m_allocator.alloc<node::Scope>();

            while (auto stmt = parse_stmt()) {
                scope->statements.push_back(stmt.value());
            }

            try_consume(TokenType::CLOSE_CURLY, "Syntax error: expected }\n");

            return scope;
        }

        return std::nullopt;
    }

    std::optional<node::Stmt*> parse_stmt() {
        // Parse exit statement
        if (try_consume(TokenType::EXIT, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtExit* stmt_exit = m_allocator.alloc<node::StmtExit>();

            if (const auto node_expr = parse_expr()) {
                stmt_exit->expression = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected integer literal after exit"
                          << "\n";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");
            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* statement = m_allocator.alloc<node::Stmt>();
            statement->var = stmt_exit;
            return statement;
        }

        // Parse print statement
        if (try_consume(TokenType::PRINT, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtPrint* stmt_print = m_allocator.alloc<node::StmtPrint>();

            if (const auto node_string_literal = parse_string_lit()) {
                stmt_print->var = node_string_literal.value();
            } else if (const auto node_expr = parse_expr()) {
                stmt_print->var = node_expr.value();
            } else {
                std::cerr << "Syntax error: expected expression after print\n";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");
            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* statement = m_allocator.alloc<node::Stmt>();
            statement->var = stmt_print;
            return statement;
        }

        // Parse let statement
        if (try_consume(TokenType::LET, false) &&
            try_consume(TokenType::IDENT, false, 1) &&
            try_consume(TokenType::EQ, false, 2)) {
            consume();
            node::StmtLet* statement_let = m_allocator.alloc<node::StmtLet>();
            statement_let->identifier = consume();
            consume();
            if (const auto node_expr = parse_expr()) {
                statement_let->expression = node_expr.value();
            } else {
                std::cerr
                    << "Syntax error: expected integer literal after let\n";
                exit(EXIT_FAILURE);
            }

            try_consume(TokenType::END_OF_LINE, "Syntax error: expected ;\n");

            node::Stmt* statement = m_allocator.alloc<node::Stmt>();
            statement->var = statement_let;
            return statement;
        }

        // Parse scope statement
        if (try_consume(TokenType::OPEN_CURLY, false)) {
            auto scope = parse_scope();
            if (!scope.has_value()) {
                std::cerr << "Syntax error: expected statement\n";
                exit(EXIT_FAILURE);
            }

            node::Stmt* statement = m_allocator.alloc<node::Stmt>();
            statement->var = scope.value();
            return statement;
        }

        // Parse if statement
        if (try_consume(TokenType::IF, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtIf* stmt_if = m_allocator.alloc<node::StmtIf>();

            auto node_expr = parse_expr();
            if (!node_expr.has_value()) {
                std::cerr << "Syntax error: expected expression after if\n";
                exit(EXIT_FAILURE);
            }
            stmt_if->condition = node_expr.value();

            try_consume(TokenType::CLOSE_PAREN, "Syntax error: expected )\n");

            auto scope = parse_scope();
            if (!scope.has_value()) {
                std::cerr << "Syntax error: expected scope after if\n";
                exit(EXIT_FAILURE);
            }
            stmt_if->scope = scope.value();

            node::Stmt* statement = m_allocator.alloc<node::Stmt>();
            statement->var = stmt_if;
            return statement;
        }

        return std::nullopt;
    }

    std::optional<node::Prog> parse_prog() {
        node::Prog prog{};
        while (peek().has_value()) {
            if (const auto statement = parse_stmt()) {
                prog.statements.push_back(statement.value());
            } else {
                std::cerr << "Syntax error: expected statement\n";
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

   private:
    [[nodiscard]] std::optional<Token> peek(const size_t offset = 0) const {
        if (m_index + offset >= m_tokens.size()) {
            return std::nullopt;
        }

        return m_tokens.at(m_index + offset);
    }

    Token consume() { return m_tokens.at(m_index++); }

    Token try_consume(const TokenType type, const std::string& error_message) {
        auto lexme = peek();
        if (lexme.has_value() && lexme.value().type == type) {
            return consume();
        }

        std::cerr << error_message << "\n";
        exit(EXIT_FAILURE);
    }

    std::optional<Token> try_consume(const TokenType type,
                                     const bool consume_token = true,
                                     const size_t offset = 0) {
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