#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

int main() {
    std::ifstream input("lab1test.cpp");
    if (!input.is_open()) {
        std::cerr << "Error: cannot open file lab1test.cpp\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string code = buffer.str();
    input.close();

    size_t openComment = code.find("/*");
    size_t closeComment = code.find("*/");
    if (openComment != std::string::npos && closeComment == std::string::npos) {
        std::cerr << "Error: unclosed multi-line comment\n";
        return 1;
    }

    try {
        std::regex multiLineComment("/\\*[\\s\\S]*?\\*/");
        code = std::regex_replace(code, multiLineComment, "");

        std::regex singleLineComment("//.*");
        code = std::regex_replace(code, singleLineComment, "");

        std::regex multipleSpaces(" {2,}");
        code = std::regex_replace(code, multipleSpaces, " ");

        std::regex emptyLines("\n{2,}");
        code = std::regex_replace(code, emptyLines, "\n");

    }
    catch (const std::regex_error&) {
        std::cerr << "Error: regular expression failure\n";
        return 1;
    }

    // trim spaces manually line by line (portable solution)
    std::stringstream cleanedStream(code);
    std::string line;
    std::string finalCode;

    while (std::getline(cleanedStream, line)) {
        size_t start = line.find_first_not_of(" \t");
        size_t end = line.find_last_not_of(" \t");

        if (start != std::string::npos && end != std::string::npos) {
            finalCode += line.substr(start, end - start + 1) + "\n";
        }
    }

    std::ofstream output("cleaned_test.cpp");
    if (!output.is_open()) {
        std::cerr << "Error: cannot create file cleaned_test.cpp\n";
        return 1;
    }

    output << finalCode;
    output.close();

    std::cout << "Source code cleaned successfully.\n";
    std::cout << "Result saved to cleaned_test.cpp\n";

    return 0;
}