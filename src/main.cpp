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

    Parser parser(std::move(tokens));
    std::optional<node::Exit> tree = parser.parse();
    if (!tree.has_value()) {
        return EXIT_FAILURE;
    }

    Generator generator(tree.value());
    {
        std::fstream output("_test/test.asm", std::ios::out);
        output << generator.generate();
    }

    return EXIT_SUCCESS;
}