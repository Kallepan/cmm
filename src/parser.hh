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

struct StmtArg {
    std::variant<Expr*, StringLit*> var;
};

struct StmtLet {
    Token identifier;
    Expr* expression;
    bool is_mutable{false};
};

struct Stmt;

struct Scope {
    std::vector<Stmt*> statements;
};

struct IfBranch {
    Expr* condition;
    Scope* scope;
};

struct ElifBranch {
    Expr* condition;
    Scope* scope;
};

struct ElseBranch {
    Scope* scope;
};

struct StmtIf {
    IfBranch* if_branch;
    std::vector<ElifBranch*> elif_branches;
    std::optional<ElseBranch*> else_branch;
};

struct StmtAssign {
    Token identifier;
    Expr* expression;
};

struct Stmt {
    std::variant<StmtExit*, StmtArg*, StmtLet*, Scope*, StmtIf*, StmtAssign*>
        var;
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
            auto term_int_lit = m_allocator.emplace<node::TermIntLit>();
            term_int_lit->integer_literal = integer_literal.value();

            auto term = m_allocator.emplace<node::Term>();
            term->var = term_int_lit;
            return term;
        }

        if (auto identifier = try_consume(TokenType::IDENT)) {
            auto term_ident = m_allocator.emplace<node::TermIdent>();
            term_ident->identifier = identifier.value();

            auto term = m_allocator.emplace<node::Term>();
            term->var = term_ident;
            return term;
        }

        if (auto open_paren = try_consume(TokenType::OPEN_PAREN)) {
            auto term_paren = m_allocator.emplace<node::TermParen>();

            if (const auto expression = parse_expr()) {
                term_paren->expression = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }

            try_consume(TokenType::CLOSE_PAREN,
                        ErrorCode::ExpectedCloseParenthesis);

            auto term = m_allocator.emplace<node::Term>();
            term->var = term_paren;
            return term;
        }

        return std::nullopt;
    }

    std::optional<node::StringLit*> parse_string_lit() {
        if (auto string_literal = try_consume(TokenType::STRING_LIT)) {
            auto string_lit = m_allocator.emplace<node::StringLit>();
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

        auto expression = m_allocator.emplace<node::Expr>();
        expression->var = term_lhs.value();

        while (true) {
            std::optional<Token> next_token = peek();
            if (!next_token.has_value()) {
                break;
            }

            std::optional<size_t> precedence =
                binary_precedence(next_token->type);
            if (!precedence.has_value() ||
                precedence.value() < minimum_precedence) {
                break;
            }

            const Token operator_token = consume();
            const size_t next_minimum_precedence = precedence.value() + 1;
            auto expr_rhs = parse_expr(next_minimum_precedence);
            if (!expr_rhs.has_value()) {
                error_expected(ErrorCode::ExpectedExpression);
            }

            auto bin_expr = m_allocator.emplace<node::BinExpr>();
            auto expr_lhs = m_allocator.emplace<node::Expr>();
            if (operator_token.type == TokenType::PLUS) {
                expr_lhs->var = expression->var;

                auto expr_binary_addition =
                    m_allocator.emplace<node::BinExprAddition>();
                expr_binary_addition->left = expr_lhs;
                expr_binary_addition->right = expr_rhs.value();

                bin_expr->var = expr_binary_addition;
            } else if (operator_token.type == TokenType::MINUS) {
                expr_lhs->var = expression->var;

                auto expr_binary_subtraction =
                    m_allocator.emplace<node::BinExprSubtraction>();
                expr_binary_subtraction->left = expr_lhs;
                expr_binary_subtraction->right = expr_rhs.value();

                bin_expr->var = expr_binary_subtraction;
            } else if (operator_token.type == TokenType::FORWARD_SLASH) {
                expr_lhs->var = expression->var;

                auto expr_binary_division =
                    m_allocator.emplace<node::BinExprDivision>();
                expr_binary_division->left = expr_lhs;
                expr_binary_division->right = expr_rhs.value();

                bin_expr->var = expr_binary_division;
            } else if (operator_token.type == TokenType::STAR) {
                expr_lhs->var = expression->var;

                auto expr_binary_multiplication =
                    m_allocator.emplace<node::BinExprMultiplication>();
                expr_binary_multiplication->left = expr_lhs;
                expr_binary_multiplication->right = expr_rhs.value();

                bin_expr->var = expr_binary_multiplication;
            } else {
                error_expected(ErrorCode::UnknownOperator);
            }

            expression->var = bin_expr;
        }

        return expression;
    }

    std::optional<node::ElseBranch*> parse_else() {
        if (try_consume(TokenType::ELSE, true)) {
            auto scope = parse_scope();
            if (!scope.has_value()) {
                error_expected(ErrorCode::ExpectedScope);
            }

            node::ElseBranch* else_branch =
                m_allocator.emplace<node::ElseBranch>();
            else_branch->scope = scope.value();

            return else_branch;
        }

        return std::nullopt;
    }

    std::vector<node::ElifBranch*> parse_elif() {
        std::vector<node::ElifBranch*> branches;
        while (try_consume(TokenType::ELIF, true)) {
            try_consume(TokenType::OPEN_PAREN,
                        ErrorCode::ExpectedOpenParenthesis);

            node::ElifBranch* elif_branch =
                m_allocator.emplace<node::ElifBranch>();
            if (const auto expression = parse_expr()) {
                elif_branch->condition = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }
            try_consume(TokenType::CLOSE_PAREN,
                        ErrorCode::ExpectedCloseParenthesis);

            auto scope = parse_scope();
            if (!scope.has_value()) {
                error_expected(ErrorCode::ExpectedScope);
            }
            elif_branch->scope = scope.value();

            branches.push_back(elif_branch);
        }

        return branches;
    }

    std::optional<node::Scope*> parse_scope() {
        if (try_consume(TokenType::OPEN_CURLY, false)) {
            consume();
            node::Scope* scope = m_allocator.emplace<node::Scope>();

            while (auto stmt = parse_stmt()) {
                scope->statements.push_back(stmt.value());
            }

            try_consume(TokenType::CLOSE_CURLY, ErrorCode::ExpectedCloseCurly);

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
            node::StmtExit* stmt_exit = m_allocator.emplace<node::StmtExit>();

            if (const auto expression = parse_expr()) {
                stmt_exit->expression = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }

            try_consume(TokenType::CLOSE_PAREN,
                        ErrorCode::ExpectedCloseParenthesis);
            try_consume(TokenType::END_OF_LINE, ErrorCode::ExpectedEndOfLine);

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
            statement->var = stmt_exit;
            return statement;
        }

        // Parse print statement
        if (try_consume(TokenType::PRINT, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtArg* statement_argument =
                m_allocator.emplace<node::StmtArg>();

            if (const auto node_string_literal = parse_string_lit()) {
                statement_argument->var = node_string_literal.value();
            } else if (const auto expression = parse_expr()) {
                statement_argument->var = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }

            try_consume(TokenType::CLOSE_PAREN,
                        ErrorCode::ExpectedCloseParenthesis);
            try_consume(TokenType::END_OF_LINE, ErrorCode::ExpectedEndOfLine);

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
            statement->var = statement_argument;
            return statement;
        }

        if (try_consume(TokenType::LET, false)) {
            consume();
            node::StmtLet* statement_let = m_allocator.emplace<node::StmtLet>();

            // Parse mutable
            if (try_consume(TokenType::MUT, false)) {
                consume();
                statement_let->is_mutable = true;
            }
            // Parse identifier
            statement_let->identifier = consume();
            consume();

            // Parse expression
            if (const auto expression = parse_expr()) {
                statement_let->expression = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }

            try_consume(TokenType::END_OF_LINE, ErrorCode::ExpectedEndOfLine);

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
            statement->var = statement_let;
            return statement;
        }

        // Assign statement
        if (try_consume(TokenType::IDENT, false) &&
            try_consume(TokenType::EQ, false, 1)) {
            auto identifier = consume();
            consume();
            node::StmtAssign* statement_assign =
                m_allocator.emplace<node::StmtAssign>();
            statement_assign->identifier = identifier;

            if (const auto expression = parse_expr()) {
                statement_assign->expression = expression.value();
            } else {
                error_expected(ErrorCode::ExpectedExpression);
            }

            try_consume(TokenType::END_OF_LINE, ErrorCode::ExpectedEndOfLine);

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
            statement->var = statement_assign;
            return statement;
        }

        // Parse scope
        if (try_consume(TokenType::OPEN_CURLY, false)) {
            auto scope = parse_scope();
            if (!scope.has_value()) {
                error_expected(ErrorCode::ExpectedScope);
            }

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
            statement->var = scope.value();
            return statement;
        }

        // Parse if statement
        if (try_consume(TokenType::IF, false) &&
            try_consume(TokenType::OPEN_PAREN, false, 1)) {
            consume();
            consume();
            node::StmtIf* stmt_if = m_allocator.emplace<node::StmtIf>();

            auto expression = parse_expr();
            if (!expression.has_value()) {
                error_expected(ErrorCode::ExpectedExpression);
            }
            node::IfBranch* if_branch = m_allocator.emplace<node::IfBranch>();
            if_branch->condition = expression.value();

            try_consume(TokenType::CLOSE_PAREN,
                        ErrorCode::ExpectedCloseParenthesis);

            auto scope = parse_scope();
            if (!scope.has_value()) {
                error_expected(ErrorCode::ExpectedScope);
            }
            if_branch->scope = scope.value();
            stmt_if->if_branch = if_branch;

            stmt_if->elif_branches = parse_elif();

            if (auto else_branch = parse_else()) {
                stmt_if->else_branch = else_branch.value();
            }

            node::Stmt* statement = m_allocator.emplace<node::Stmt>();
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
                error_expected(ErrorCode::InvalidProgram);
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

    void error_expected(const ErrorCode error_code) const {
        std::cerr << ErrorManager::construct_error_message(
            error_code, peek()->line_number, peek()->col_number);
        exit(EXIT_FAILURE);
    }

    Token try_consume(const TokenType type, const ErrorCode error_code) {
        auto lexme = peek();
        if (lexme.has_value() && lexme.value().type == type) {
            return consume();
        }

        std::cerr << ErrorManager::construct_error_message(
            error_code, lexme->line_number, lexme->col_number);
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