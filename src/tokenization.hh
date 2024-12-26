#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "token_type.hh"

struct Token {
    TokenType type;
    std::string value;
};

class Tokenizer {
   public:
    inline explicit Tokenizer(const std::string src) : m_src(std::move(src)) {}

    inline std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string token_buff;

        // Helper function to consume characters while a condition is met
        auto consume_while = [&](auto condition) {
            while (peek().has_value() && condition(peek().value())) {
                token_buff.push_back(consume());
            }
        };

        // Continue looking for tokens until the end of the source
        while (peek().has_value()) {
            char c = peek().value();

            // New line
            if (c == '\n') {
                m_line_number++;
                m_col_number = 0;
                consume();
                continue;
            }

            // Skip whitespace
            if (std::isspace(c)) {
                consume();
                continue;
            }

            // Identifier
            if (std::isalpha(c)) {
                token_buff.push_back(consume());
                consume_while([](char c) { return std::isalnum(c); });

                // Handle keywords
                if (token_buff == "exit") {
                    tokens.push_back({TokenType::EXIT, token_buff});
                    token_buff.clear();
                    continue;
                }
                if (token_buff == "let") {
                    tokens.push_back({TokenType::LET, token_buff});
                    token_buff.clear();
                    continue;
                }

                tokens.push_back({TokenType::IDENT, token_buff});
                token_buff.clear();
                continue;
            }

            // Numbers
            if ((c == '-' && std::isdigit(peek(1).value())) ||
                std::isdigit(c)) {
                token_buff.push_back(consume());
                consume_while([](char c) { return std::isdigit(c); });

                tokens.push_back({TokenType::INT_LIT, token_buff});
                token_buff.clear();
                continue;
            }

            // Handle operators
            if (c == '(') {
                consume();
                tokens.push_back({TokenType::OPEN_PAREN, "("});
                continue;
            }
            if (c == ')') {
                consume();
                tokens.push_back({TokenType::CLOSE_PAREN, ")"});
                continue;
            }
            if (c == '=') {
                consume();
                tokens.push_back({TokenType::EQ, "="});
                continue;
            }
            if (c == '+') {
                consume();
                tokens.push_back({TokenType::PLUS, "+"});
                continue;
            }
            if (c == '-') {
                consume();
                tokens.push_back({TokenType::MINUS, "-"});
                continue;
            }
            if (c == '*') {
                consume();
                tokens.push_back({TokenType::STAR, "*"});
                continue;
            }
            if (c == '/') {
                consume();
                tokens.push_back({TokenType::FORWARD_SLASH, "/"});
                continue;
            }

            // Handle braces
            if (c == '{') {
                consume();
                tokens.push_back({TokenType::OPEN_CURLY, "{"});
                continue;
            }
            if (c == '}') {
                consume();
                tokens.push_back({TokenType::CLOSE_CURLY, "}"});
                continue;
            }

            // Semicolon, end of line
            if (c == ';') {
                consume();
                tokens.push_back({TokenType::END_OF_LINE, ";"});
                continue;
            }

            // Syntax error, no token found
            std::cerr << "Syntax error: " << peek().value()
                      << " at column: " << m_col_number
                      << " in line: " << m_line_number << "\n";
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.type << ", Value: " << token.value
                      << "\n";
        }
#endif
        m_index = 0;
        m_col_number = 1;
        m_line_number = 1;
        return tokens;
    }

   private:
    [[nodiscard]] inline std::optional<char> peek(size_t offset = 0) const {
        if (m_index + offset >= m_src.size()) {
            return std::nullopt;
        }

        return m_src[m_index + offset];
    }

    inline char consume() {
        m_col_number++;
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index{0};
    size_t m_col_number{1};
    size_t m_line_number{1};
};