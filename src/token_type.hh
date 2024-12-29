#pragma once

enum class TokenType {
    INT_LIT = 0,  // 123
    STRING_LIT,   // "abc"
    IDENT,        // abc

    LET,  // let
    MUT,  // let mut

    EXIT,   // return
    PRINT,  // print

    OPEN_PAREN,     // (
    CLOSE_PAREN,    // )
    EQ,             // =
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    FORWARD_SLASH,  // /

    OPEN_CURLY,   // {
    CLOSE_CURLY,  // }

    IF,    // if
    ELIF,  // elif
    ELSE,  // else

    END_OF_LINE,  // ;
};

/**
 * @brief Convert a TokenType to a string
 */
constexpr const char* toString(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::STRING_LIT:
            return "String Literal";
        case TokenType::INT_LIT:
            return "Integer Literal";
        case TokenType::IDENT:
            return "Identifier";

        case TokenType::LET:
            return "`let`";
        case TokenType::MUT:
            return "`mut`";

        case TokenType::EXIT:
            return "`exit`";
        case TokenType::PRINT:
            return "`print`";

        case TokenType::OPEN_PAREN:
            return "`(`";
        case TokenType::CLOSE_PAREN:
            return "`)`";
        case TokenType::OPEN_CURLY:
            return "`{`";
        case TokenType::CLOSE_CURLY:
            return "`}`";

        case TokenType::EQ:
            return "`=`";
        case TokenType::PLUS:
            return "`+`";
        case TokenType::MINUS:
            return "`-`";
        case TokenType::STAR:
            return "`*`";
        case TokenType::FORWARD_SLASH:
            return "`/`";

        case TokenType::IF:
            return "`if`";
        case TokenType::ELIF:
            return "`elif`";
        case TokenType::ELSE:
            return "`else`";

        case TokenType::END_OF_LINE:
            return "`;`";

        default:
            return "Unknown";
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
inline std::optional<size_t> binary_precedence(const TokenType tokenType) {
    switch (tokenType) {
        case TokenType::PLUS:
        case TokenType::MINUS:
            return 1;
        case TokenType::STAR:
        case TokenType::FORWARD_SLASH:
            return 2;
        default:
            return std::nullopt;
    }
}