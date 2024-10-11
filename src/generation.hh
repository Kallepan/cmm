#pragma once

#include "parser.hh"

class Generator {
   public:
    inline explicit Generator(node::Exit root) : m_root(std::move(root)) {}

    [[nodiscard]] std::string generate() const {
        std::stringstream output;
        output << "global _start\n\n_start:\n";

        output << "    mov rax, 60\n";
        output << "    mov rdi, " << m_root.expr.int_lit.value << "\n";
        output << "    syscall\n";

        return output.str();
    }

   private:
    const node::Exit m_root;
};