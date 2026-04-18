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

vector<string> errors;

void addError(const string& text, int pos) {
    errors.push_back("Ошибка на позиции " + to_string(pos) + ": " + text);
}

// Ключевые слова
bool isKeyword(const string& s) {
    return (s == "int" || s == "float" || s == "if" || s == "else" ||
        s == "return" || s == "using" || s == "namespace");
}

bool isDelimiter(char c) {
    return (c == ';' || c == ',' || c == '(' || c == ')' || c == '{' || c == '}');
}

bool isOperatorChar(char c) {
    return string("+-*/=<>&|!").find(c) != string::npos;
}

// Проверка, есть ли идентификатор в списке разрешённых
bool isKnownIdentifier(const string& s, const vector<string>& identifiers) {
    for (const auto& id : identifiers) {
        if (id == s) return true;
    }
    return false;
}

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

vector<Token> tokenize(const string& code) {
    vector<Token> tokens;

    // Изначально известные идентификаторы
    vector<string> identifiers = { "std", "cout", "endl" };

    string current;
    errors.clear();

    for (int i = 0; i < (int)code.length(); i++) {

        if (isspace((unsigned char)code[i])) continue;

        if (code[i] == '#') {
            while (i < (int)code.length() && code[i] != '\n') i++;
            continue;
        }

        if (code[i] == '/' && i + 1 < (int)code.length() && code[i + 1] == '/') {
            while (i < (int)code.length() && code[i] != '\n') i++;
            continue;
        }

        if (code[i] == '/' && i + 1 < (int)code.length() && code[i + 1] == '*') {
            int commentStart = i + 1;
            i += 2;

            while (i + 1 < (int)code.length() && !(code[i] == '*' && code[i + 1] == '/')) i++;

            if (i + 1 >= (int)code.length()) {
                addError("незакрытый комментарий", commentStart);
                break;
            }

            i++;
            continue;
        }

        // строки
        if (code[i] == '"') {
            int stringStart = i + 1;
            current = "";
            i++;

            while (i < (int)code.length() && code[i] != '"') {
                current += code[i++];
            }

            if (i >= (int)code.length()) {
                addError("незакрытая строка", stringStart);
            }
            else {
                tokens.push_back({ "STRING", current });
            }
            continue;
        }

        // слова
        if (isalpha((unsigned char)code[i])) {
            int wordStart = i + 1;
            current = "";

            while (i < (int)code.length() && isalnum((unsigned char)code[i])) {
                current += code[i++];
            }
            i--;

            if (isKeyword(current)) {
                tokens.push_back({ "KEYWORD", current });
            }
            else {
                // если перед словом был тип int или float, считаем это объявлением идентификатора
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                bool afterDeclarationKeyword = !tokens.empty() &&
                    tokens.back().type == "KEYWORD" &&
                    (tokens.back().value == "int" || tokens.back().value == "float");


                if (afterDeclarationKeyword) {
                    if (!isKnownIdentifier(current, identifiers)) {
                        identifiers.push_back(current);
                    }
                    tokens.push_back({ "IDENTIFIER", current });
                }
                else if (isKnownIdentifier(current, identifiers)) {
                    tokens.push_back({ "IDENTIFIER", current });
                }
                else {
                    addError("неизвестный идентификатор: " + current, wordStart);
                }
            }
        }

        // числа
        else if (isdigit((unsigned char)code[i])) {
            current = "";
            bool hasDot = false;
            bool hasError = false;

            while (i < (int)code.length() &&
                (isdigit((unsigned char)code[i]) || code[i] == '.' || code[i] == ',')) {

                if (code[i] == '.') {
                    if (hasDot) {
                        addError("некорректное число: лишняя точка", i + 1);
                        hasError = true;
                    }
                    hasDot = true;
                }

                if (code[i] == ',') {
                    addError("в числе используется запятая вместо точки", i + 1);
                    hasError = true;
                }

                current += code[i++];
            }

            if (i < (int)code.length() && isalpha((unsigned char)code[i])) {
                addError("буквы в числе: " + current + code[i], i + 1);
                hasError = true;
            }

            i--;

            if (!hasError) {
                if (hasDot)
                    tokens.push_back({ "CONSTANT_FLOAT", current });
                else
                    tokens.push_back({ "CONSTANT_INT", current });
            }
        }

        else if (isOperatorChar(code[i])) {
            current = "";
            current += code[i];

            if (i + 1 < (int)code.length() && isOperatorChar(code[i + 1])) {
                current += code[i + 1];
                i++;
            }

            tokens.push_back({ "OPERATOR", current });
        }

        else if (isDelimiter(code[i])) {
            tokens.push_back({ "DELIMITER", string(1, code[i]) });
        }

        else {
            addError("недопустимый символ: " + string(1, code[i]), i + 1);
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

    cout << "\nСписок токенов:\n[";
    for (int i = 0; i < (int)tokens.size(); i++) {
        cout << "(" << tokens[i].type << ", " << tokens[i].value << ")";
        if (i != (int)tokens.size() - 1) cout << ", ";
    }
    cout << "]\n";

    cout << "\nРезультат анализа:\n";

    if (errors.empty()) {
        cout << "Лексический анализ завершён успешно. Ошибок не найдено.\n";
    }
    else {
        cout << "Обнаружены ошибки:\n";
        for (auto e : errors) {
            cout << e << endl;
        }
    }

    return 0;
}
