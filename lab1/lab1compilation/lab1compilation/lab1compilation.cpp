#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

int main() {
    // тестовый файл напиши
    std::ifstream input("lab1test.cpp");
    if (!input.is_open()) {
        std::cerr << "Error: cannot open file lab1test.cpp\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string code = buffer.str();
    input.close();

    // незакрытый многострочный комментарий
    size_t openComment = code.find("/*");
    size_t closeComment = code.find("*/");
    if (openComment != std::string::npos && closeComment == std::string::npos) {
        std::cerr << "Error: unclosed multi-line comment\n";
        return 1;
    }

    try {
        // регулярки закинь
    }
    catch (const std::regex_error&) {
        std::cerr << "Error: regular expression failure\n";
        return 1;
    }

    std::ofstream output("cleaned_test.cpp");
    if (!output.is_open()) {
        std::cerr << "Error: cannot create file cleaned_test.cpp\n";
        return 1;
    }

    output << code;
    output.close();

    std::cout << "Source code cleaned successfully.\n";
    std::cout << "Result saved to cleaned_test.cpp\n";

    return 0;
}