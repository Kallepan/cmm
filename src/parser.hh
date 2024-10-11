#pragma once

#include <utility>
#include <vector>

#include "tokenization.hh"

namespace node {
struct Expr {
    Token int_lit;
};

struct Exit {
    Expr expr;
};
}  // namespace node

class Parser {
   public:
    inline explicit Parser(std::vector<Token> tokens)
        : m_tokens(std::move(tokens)) {}

    std::optional<node::Expr> parse_expr() {
        if (peek().has_value() && peek().value().type == TokenType::INT_LIT) {
            return node::Expr{consume()};
        }

        return std::nullopt;
    }

    std::optional<node::Exit> parse() {
        std::optional<node::Exit> exit_node;

        while (peek().has_value()) {
            if (peek().value().type == TokenType::EXIT) {
                consume();
                if (auto node_expr = parse_expr()) {
                    exit_node = node::Exit{node_expr.value()};
                } else {
                    std::cerr
                        << "Syntax error: expected integer literal after exit"
                        << std::endl;
                    return std::nullopt;
                }

                if (peek().has_value() ||
                    peek().value().type == TokenType::END_OF_LINE) {
                    consume();

                } else {
                    std::cerr << "Syntax error: expected ; after exit"
                              << std::endl;
                    return std::nullopt;
                }
            }
        }

        m_index = 0;
        return exit_node;
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