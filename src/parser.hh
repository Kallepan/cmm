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
        if (peak().has_value() && peak().value().type == TokenType::INT_LIT) {
            return node::Expr{consume()};
        }

        return std::nullopt;
    }

    std::optional<node::Exit> parse() {
        std::optional<node::Exit> exit_node;

        while (peak().has_value()) {
            if (peak().value().type == TokenType::EXIT) {
                consume();
                if (auto node_expr = parse_expr()) {
                    exit_node = node::Exit{node_expr.value()};
                } else {
                    std::cerr
                        << "Syntax error: expected integer literal after exit"
                        << std::endl;
                    return std::nullopt;
                }

                if (peak().has_value() ||
                    peak().value().type == TokenType::END_OF_LINE) {
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
    [[nodiscard]] inline std::optional<Token> peak(size_t ahead = 0) const {
        if (m_index + ahead >= m_tokens.size()) {
            return std::nullopt;
        }

        return m_tokens.at(m_index + ahead);
    }

    inline Token consume() { return m_tokens.at(m_index++); }

    const std::vector<Token> m_tokens;
    size_t m_index{0};
};