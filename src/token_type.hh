#pragma once

enum class TokenType {
    INT_LIT = 0,
    // 123

    EXIT,  // return

    END_OF_LINE,  // ;
    END_OF_FILE,  // EOF
};

constexpr const char* toString(TokenType tokenType) {
    switch (tokenType) {
        case TokenType::INT_LIT:
            return "INT_LIT";
        case TokenType::EXIT:
            return "EXIT";
        case TokenType::END_OF_LINE:
            return "END_OF_LINE";
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
        default:
            return "UNKNOWN";
    }
}

inline std::ostream& operator<<(std::ostream& os, TokenType tokenType) {
    return os << toString(tokenType);
}