#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "config.hh"
#include "error.hh"
#include "token_type.hh"

struct Token {
    TokenType type;
    std::string value;
    size_t line_number;
    size_t col_number;
};

class Tokenizer {
   public:
    explicit Tokenizer(const std::string src) : m_src(std::move(src)) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        std::string token_buff;

        // Continue looking for tokens until the end of the source
        while (peek().has_value()) {
            char c = peek().value();

            // New line
            if (c == '\n') {
                handle_new_line();
                consume();
                continue;
            }

            // Skip whitespace
            if (std::isspace(c)) {
                consume();
                continue;
            }

            // Comments
            if (c == '/' && peek(1).has_value() && peek(1).value() == '/') {
                consume();
                consume();
                while (peek().has_value() && peek().value() != '\n') {
                    consume();
                }
                continue;
            }
            if (c == '/' && peek(1).has_value() && peek(1).value() == '*') {
                consume();
                consume();
                while (peek().has_value()) {
                    if (peek().value() == '*' && peek(1).value() == '/') {
                        consume();
                        consume();
                        break;
                    }

                    // Handle new lines in comments
                    if (peek().value() == '\n') {
                        handle_new_line();
                    }

                    consume();
                }
                continue;
            }

            // Identifier
            if (std::isalpha(c)) {
                token_buff.push_back(consume());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    token_buff.push_back(consume());
                }

                // Handle keywords
                if (token_buff == "exit") {
                    tokens.push_back({TokenType::EXIT, token_buff,
                                      m_line_number, m_col_number});
                    token_buff.clear();
                    continue;
                }

                if (token_buff == "print") {
                    tokens.push_back({TokenType::PRINT, token_buff,
                                      m_line_number, m_col_number});
                    token_buff.clear();
                    continue;
                }

                if (token_buff == "let") {
                    tokens.push_back({TokenType::LET, token_buff, m_line_number,
                                      m_col_number});
                    token_buff.clear();
                    continue;
                }

                if (token_buff == "if") {
                    tokens.push_back({TokenType::IF, token_buff, m_line_number,
                                      m_col_number});
                    token_buff.clear();
                    continue;
                }
                if (token_buff == "elif") {
                    tokens.push_back({TokenType::ELIF, token_buff,
                                      m_line_number, m_col_number});
                    token_buff.clear();
                    continue;
                }
                if (token_buff == "else") {
                    tokens.push_back({TokenType::ELSE, token_buff,
                                      m_line_number, m_col_number});
                    token_buff.clear();
                    continue;
                }

                if (token_buff == "mut") {
                    tokens.push_back({TokenType::MUT, token_buff, m_line_number,
                                      m_col_number});
                    token_buff.clear();
                    continue;
                }

                tokens.push_back({TokenType::IDENT, token_buff, m_line_number,
                                  m_col_number});
                token_buff.clear();
                continue;
            }

            // Numbers
            if ((c == '-' && std::isdigit(peek(1).value())) ||
                std::isdigit(c)) {
                token_buff.push_back(consume());
                while (peek().has_value()) {
                    if (peek().value() == '_' && peek(1).has_value() &&
                        std::isdigit(peek(1).value())) {
                        consume();
                        continue;
                    }
                    if (!std::isdigit(peek().value())) {
                        break;
                    }
                    token_buff.push_back(consume());
                }

                tokens.push_back({TokenType::INT_LIT, token_buff, m_line_number,
                                  m_col_number});
                token_buff.clear();
                continue;
            }

            // Strings
            if (c == '"') {
                consume();
                while (peek().has_value() && peek().value() != '"') {
                    if (peek().value() == '\\' && peek(1).has_value()) {
                        consume();

                        // Handle escape characters
                        if (peek().value() == 'n') {
                            consume();
                            token_buff.push_back('\n');
                            continue;
                        }

                        continue;
                    }

                    token_buff.push_back(consume());
                }
                consume();
                if (token_buff.size() > MAX_STRING_SIZE) {
                    std::cerr << ErrorManager::construct_error_message(
                        ErrorCode::StringTooLong, m_line_number, m_col_number);

                    exit(EXIT_FAILURE);
                }
                tokens.push_back({TokenType::STRING_LIT, token_buff,
                                  m_line_number, m_col_number});
                token_buff.clear();
                continue;
            }

            // Handle operators
            if (c == '(') {
                consume();
                tokens.push_back(
                    {TokenType::OPEN_PAREN, "(", m_line_number, m_col_number});
                continue;
            }
            if (c == ')') {
                consume();
                tokens.push_back(
                    {TokenType::CLOSE_PAREN, ")", m_line_number, m_col_number});
                continue;
            }
            if (c == '=') {
                consume();
                tokens.push_back(
                    {TokenType::EQ, "=", m_line_number, m_col_number});
                continue;
            }
            if (c == '+') {
                consume();
                tokens.push_back(
                    {TokenType::PLUS, "+", m_line_number, m_col_number});
                continue;
            }
            if (c == '-') {
                consume();
                tokens.push_back(
                    {TokenType::MINUS, "-", m_line_number, m_col_number});
                continue;
            }
            if (c == '*') {
                consume();
                tokens.push_back(
                    {TokenType::STAR, "*", m_line_number, m_col_number});
                continue;
            }
            if (c == '/') {
                consume();
                tokens.push_back({TokenType::FORWARD_SLASH, "/", m_line_number,
                                  m_col_number});
                continue;
            }

            // Handle braces
            if (c == '{') {
                consume();
                tokens.push_back(
                    {TokenType::OPEN_CURLY, "{", m_line_number, m_col_number});
                continue;
            }
            if (c == '}') {
                consume();
                tokens.push_back(
                    {TokenType::CLOSE_CURLY, "}", m_line_number, m_col_number});
                continue;
            }

            // Semicolon, end of line
            if (c == ';') {
                consume();
                tokens.push_back(
                    {TokenType::END_OF_LINE, ";", m_line_number, m_col_number});
                continue;
            }

            // Syntax error, no token found
            std::cerr << ErrorManager::construct_error_message(
                ErrorCode::UnidentifiedToken, m_line_number, m_col_number);
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.type << ", Value: `" << token.value
                      << "`, Line: " << token.line_number
                      << ", Column: " << token.col_number << "\n";
        }

        std::cout << "Tokenization complete\n"
                  << "file had " << m_line_number << " lines\n";
#endif
        m_index = 0;
        return tokens;
    }

   private:
    [[nodiscard]] std::optional<char> peek(size_t offset = 0) const {
        if (m_index + offset >= m_src.size()) {
            return std::nullopt;
        }

        return m_src[m_index + offset];
    }

    char consume() {
        m_col_number++;
        return m_src.at(m_index++);
    }

    void handle_new_line() {
#ifdef DEBUG
        std::cout << "New line at: " << m_line_number << " with "
                  << m_col_number << " columns\n";
#endif
        m_line_number++;
        m_col_number = 0;
    }

    const std::string m_src;
    size_t m_index{0};
    size_t m_col_number{0};
    size_t m_line_number{1};
};