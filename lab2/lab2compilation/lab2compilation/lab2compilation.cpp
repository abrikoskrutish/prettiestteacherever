#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>

using namespace std;

struct Token {
    string type;
    string value;
};

// Ключевые слова
bool isKeyword(string s) {
    return (s == "int" || s == "if" || s == "else" || s == "return" || s == "using" || s == "namespace");
}

// Разделители
bool isDelimiter(char c) {
    return (c == ';' || c == ',' || c == '(' || c == ')' || c == '{' || c == '}');
}

// Операторы
bool isOperatorChar(char c) {
    return string("+-*/=<>&|!").find(c) != string::npos;
}

// Чтение файла + удаление BOM
string readFile(string filename) {
    ifstream file(filename, ios::binary);
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3);
    }

    return content;
}

vector<Token> tokenize(string code) {
    vector<Token> tokens;
    string current;

    for (int i = 0; i < code.length(); i++) {

        if (isspace(code[i])) continue;

        // #include
        if (code[i] == '#') {
            while (i < code.length() && code[i] != '\n') i++;
            continue;
        }

        // комментарии
        if (code[i] == '/' && i + 1 < code.length() && code[i + 1] == '/') {
            while (i < code.length() && code[i] != '\n') i++;
            continue;
        }

        if (code[i] == '/' && i + 1 < code.length() && code[i + 1] == '*') {
            i += 2;
            while (i + 1 < code.length() && !(code[i] == '*' && code[i + 1] == '/')) i++;
            i++;
            continue;
        }

        // строки
        if (code[i] == '"') {
            current = "";
            i++;
            while (i < code.length() && code[i] != '"') {
                current += code[i++];
            }
            tokens.push_back({ "STRING", current });
            continue;
        }

        // слова
        if (isalpha(code[i])) {
            current = "";
            while (i < code.length() && isalnum(code[i])) {
                current += code[i++];
            }
            i--;

            if (isKeyword(current))
                tokens.push_back({ "KEYWORD", current });
            else
                tokens.push_back({ "IDENTIFIER", current });
        }

        // числа
        else if (isdigit(code[i])) {
            current = "";
            while (i < code.length() && isdigit(code[i])) {
                current += code[i++];
            }
            i--;

            tokens.push_back({ "CONSTANT_INT", current });
        }

        // операторы
        else if (isOperatorChar(code[i])) {
            current = "";
            current += code[i];

            if (i + 1 < code.length() && isOperatorChar(code[i + 1])) {
                current += code[i + 1];
                i++;
            }

            tokens.push_back({ "OPERATOR", current });
        }

        // разделители
        else if (isDelimiter(code[i])) {
            tokens.push_back({ "DELIMITER", string(1, code[i]) });
        }
    }

    return tokens;
}

int main() {

    setlocale(LC_ALL, "Russian");

    string code = readFile("test.cpp");
    vector<Token> tokens = tokenize(code);

    cout << "Лексема\t\tТип\n";
    cout << "--------------------------\n";

    for (auto t : tokens) {
        cout << t.value << "\t\t" << t.type << endl;
    }

    return 0;
}