#include "main.hh"

std::string tokens_to_asm(const std::vector<Token>& tokens) {
    std::stringstream output;
    output << "global _start\n\n_start:\n";

    for (int i{0}; i < tokens.size(); ++i) {
        const Token& token = tokens[i];

        if (token.type == TokenType::EXIT) {
            if (tokens[i + 1].type != TokenType::INTEGER_LITERAL) {
                std::cerr << "Syntax error: expected integer literal after exit"
                          << std::endl;
                return "";
            }

            output << "    mov rax, 60\n";
            output << "    mov rdi, " << tokens[i + 1].value << "\n";
            output << "    syscall\n";
        }
    }

    return output.str();
}