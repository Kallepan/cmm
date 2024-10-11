#include "main.hh"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct usage:" << std::endl;
        std::cerr << "cmm <filename>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Error opening file: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
    std::string contents;
    {
        std::stringstream buffer;
        buffer << input.rdbuf();
        contents = buffer.str();
    }

    Tokenizer tokenizer(std::move(contents));
    std::vector<Token> tokens = tokenizer.tokenize();

    {
        std::fstream output("_test/test.asm", std::ios::out);
        output << tokens_to_asm(tokens);
    }

    return EXIT_SUCCESS;
}