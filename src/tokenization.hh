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

        // Continue looking for tokens until the end of the source
        while (peek().has_value()) {
            // Handle Newline
            if (peek().value() == '\n') {
                m_line_number++;
                consume();
                continue;
            } else if (std::isalpha(peek().value())) {
                token_buff.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    token_buff.push_back(consume());
                }

                // Handle token
                if (token_buff == "exit") {
                    tokens.push_back({TokenType::EXIT, token_buff});
                    token_buff.clear();
                    continue;
                } else if (token_buff == "let") {
                    tokens.push_back({TokenType::LET, token_buff});
                    token_buff.clear();
                    continue;
                }

                tokens.push_back({TokenType::IDENT, token_buff});
                token_buff.clear();
                continue;

            } else if (std::isdigit(peek().value())) {
                token_buff.push_back(consume());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    token_buff.push_back(consume());
                }

                // Handle token
                if (std::isdigit(token_buff[0])) {
                    tokens.push_back({TokenType::INT_LIT, token_buff});
                    token_buff.clear();
                    continue;
                } else {
                    std::cerr << "Syntax error: " << token_buff
                              << " at line: " << m_line_number << "\n";

                    exit(EXIT_FAILURE);
                }
            } else if (peek().value() == '(') {
                consume();
                tokens.push_back({TokenType::OPEN_PAREN, "("});
                continue;
            } else if (peek().value() == ')') {
                consume();
                tokens.push_back({TokenType::CLOSE_PAREN, ")"});
                continue;
            } else if (peek().value() == '=') {
                consume();
                tokens.push_back({TokenType::EQ, "="});
                continue;
            } else if (peek().value() == '+') {
                consume();
                tokens.push_back({TokenType::PLUS, "+"});
                continue;
            } else if (peek().value() == '*') {
                consume();
                tokens.push_back({TokenType::MULTIPLY, "*"});
                continue;
            } else if (peek().value() == ';') {
                consume();
                tokens.push_back({TokenType::END_OF_LINE, ";"});
                continue;
            } else if (std::isspace(peek().value())) {
                consume();
                continue;
            } else {
                std::cerr << "Syntax error: " << peek().value()
                          << " at line: " << m_line_number << "\n";
                exit(EXIT_FAILURE);
            }
        }

#ifdef DEBUG
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.type << ", Value: " << token.value
                      << "\n";
        }
#endif
        m_index = 0;
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

    inline char consume() { return m_src.at(m_index++); }

    const std::string m_src;
    size_t m_index{0};
    size_t m_line_number{1};
};