#include "main.hh"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage. Correct usage:\n";
        std::cerr << "cmm <filename>\n";
        return EXIT_FAILURE;
    }

    std::ifstream input(argv[1]);
    if (!input) {
        std::cerr << "Error opening file: " << argv[1] << "\n";
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

    Parser parser(std::move(tokens));
    std::optional<node::Prog> prog = parser.parse_prog();
    if (!prog.has_value()) {
        std::cerr << "Invalid Program\n";
        return EXIT_FAILURE;
    }

    Generator generator(prog.value());
    {
        std::fstream output("_test/test.asm", std::ios::out);
        output << generator.gen_prog();
    }

    return EXIT_SUCCESS;
}