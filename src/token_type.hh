#pragma once

enum class TokenType {
    INT_LIT = 0,  // 123
    IDENT,        // abc

    LET,  // let

    EXIT,  // return

    OPEN_PAREN,   // (
    CLOSE_PAREN,  // )
    EQ,           // =
    PLUS,         // +
    SUBTRACT,     // -
    MULTIPLY,     // *
    DIVIDE,       // /

    END_OF_LINE,  // ;
};

constexpr const char* toString(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::INT_LIT:
            return "INT_LIT";
        case TokenType::IDENT:
            return "IDENT";

        case TokenType::LET:
            return "LET";
        case TokenType::EXIT:
            return "EXIT";

        case TokenType::OPEN_PAREN:
            return "OPEN_PARENTHESES";
        case TokenType::CLOSE_PAREN:
            return "CLOSE_PARENTHESIS";
        case TokenType::EQ:
            return "EQUALS";
        case TokenType::PLUS:
            return "PLUS";
        case TokenType::SUBTRACT:
            return "SUBTRACT";
        case TokenType::MULTIPLY:
            return "MULTIPLY";
        case TokenType::DIVIDE:
            return "DIVIDE";

        case TokenType::END_OF_LINE:
            return "END_OF_LINE";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Overload the << operator for TokenType to return the string
 * representation
 */
inline std::ostream& operator<<(std::ostream& os, TokenType tokenType) {
    return os << toString(tokenType);
}

/**
 * @brief Check if a token has a unary precedence
 */
std::optional<int> bin_prec(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::PLUS:
        case TokenType::SUBTRACT:
            return 1;
        case TokenType::MULTIPLY:
        case TokenType::DIVIDE:
            return 2;
        default:
            return std::nullopt;
    }
}