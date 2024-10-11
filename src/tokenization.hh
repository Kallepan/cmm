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
        while (peak().has_value()) {
            // Handle Newline
            if (peak().value() == '\n') {
                m_line_number++;
                consume();
                continue;
            } else if (std::isalpha(peak().value())) {
                token_buff.push_back(consume());
                while (peak().has_value() && std::isalnum(peak().value())) {
                    token_buff.push_back(consume());
                }

                // Handle token
                if (token_buff == "exit") {
                    tokens.push_back({TokenType::EXIT, token_buff});
                    token_buff.clear();
                    continue;
                } else {
                    std::cerr << "Syntax error: " << token_buff
                              << " at line: " << m_line_number << std::endl;
                    return {};
                }

            } else if (std::isdigit(peak().value())) {
                token_buff.push_back(consume());
                while (peak().has_value() && std::isdigit(peak().value())) {
                    token_buff.push_back(consume());
                }

                // Handle token
                if (std::isdigit(token_buff[0])) {
                    tokens.push_back({TokenType::INTEGER_LITERAL, token_buff});
                    token_buff.clear();
                    continue;
                } else {
                    std::cerr << "Syntax error: " << token_buff
                              << " at line: " << m_line_number << std::endl;

                    return {};
                }
            } else if (peak().value() == ';') {
                consume();
                tokens.push_back({TokenType::END_OF_LINE, ";"});
                continue;
            } else if (std::isspace(peak().value())) {
                consume();
                continue;
            } else {
                std::cerr << "Syntax error: " << peak().value()
                          << " at line: " << m_line_number << std::endl;
                return {};
            }
        }

#ifdef DEBUG
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.type << ", Value: " << token.value
                      << std::endl;
        }
#endif
        m_index = 0;
        m_line_number = 1;
        return tokens;
    }

   private:
    [[nodiscard]] inline std::optional<char> peak(size_t ahead = 0) const {
        if (m_index + ahead >= m_src.size()) {
            return std::nullopt;
        }

        return m_src[m_index + ahead];
    }

    inline char consume() { return m_src.at(m_index++); }

    const std::string m_src;
    size_t m_index{0};
    size_t m_line_number{1};
};