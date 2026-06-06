#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>

using namespace std;

struct Token {
    string type;
    string value;
    int index;
};

struct ParserError {
    string message;
    int tokenIndex;
    string tokenValue;
};

struct ASTNode {
    string name;
    string value;
    vector<shared_ptr<ASTNode>> children;

    ASTNode(const string& n, const string& v = "") : name(n), value(v) {}

    void add(shared_ptr<ASTNode> child) {
        if (child) children.push_back(child);
    }
};

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Ошибка: не удалось открыть файл " << filename << endl;
        return "";
    }

    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void skipSpaces(const string& text, size_t& pos) {
    while (pos < text.size() && isspace(static_cast<unsigned char>(text[pos]))) {
        pos++;
    }
}

vector<Token> readTokensFromFile(const string& filename, vector<string>& formatErrors) {
    string text = readFile(filename);
    vector<Token> tokens;

    size_t pos = 0;
    int tokenIndex = 1;

    skipSpaces(text, pos);

    if (pos >= text.size() || text[pos] != '[') {
        formatErrors.push_back("Поток токенов должен начинаться с символа [");
        return tokens;
    }
    pos++;

    while (pos < text.size()) {
        skipSpaces(text, pos);

        if (pos < text.size() && text[pos] == ']') {
            pos++;
            break;
        }

        if (pos >= text.size() || text[pos] != '(') {
            formatErrors.push_back("Ожидался символ ( в позиции " + to_string(pos + 1));
            break;
        }
        pos++;

        size_t typeStart = pos;
        while (pos < text.size() && text[pos] != ',') {
            pos++;
        }

        if (pos >= text.size()) {
            formatErrors.push_back("Не найдена запятая после типа токена");
            break;
        }

        string type = trim(text.substr(typeStart, pos - typeStart));
        pos++; // пропускаем запятую
        skipSpaces(text, pos);

        string value;
        bool closed = false;

        while (pos < text.size()) {
            if (text[pos] == ')') {
                size_t checkPos = pos + 1;
                skipSpaces(text, checkPos);

                // Это закрывающая скобка кортежа токена, если после нее идет запятая или конец списка.
                if (checkPos < text.size() && (text[checkPos] == ',' || text[checkPos] == ']')) {
                    closed = true;
                    pos++;
                    break;
                }
            }

            value += text[pos];
            pos++;
        }

        if (!closed) {
            formatErrors.push_back("Не найдена закрывающая скобка ) для токена " + to_string(tokenIndex));
            break;
        }

        tokens.push_back({ type, trim(value), tokenIndex });
        tokenIndex++;

        skipSpaces(text, pos);
        if (pos < text.size() && text[pos] == ',') {
            pos++;
        }
    }

    tokens.push_back({ "EOF", "EOF", tokenIndex });
    return tokens;
}

class Parser {
private:
    vector<Token> tokens;
    size_t pos;
    vector<ParserError> errors;

    Token current() const {
        if (pos < tokens.size()) return tokens[pos];
        return tokens.back();
    }

    bool check(const string& type, const string& value = "") const {
        Token t = current();
        if (t.type != type) return false;
        return value.empty() || t.value == value;
    }

    bool match(const string& type, const string& value = "") {
        if (check(type, value)) {
            pos++;
            return true;
        }
        return false;
    }

    void addError(const string& expected) {
        Token t = current();
        errors.push_back({
            "Получено '" + t.value + "' (" + t.type + "), ожидалось: " + expected,
            t.index,
            t.value
            });
    }

    bool expect(const string& type, const string& value, const string& expectedText) {
        if (match(type, value)) return true;
        addError(expectedText);
        return false;
    }

    void skipBadToken() {
        if (!check("EOF")) pos++;
    }

    bool isTypeKeyword() const {
        return check("KEYWORD", "int") || check("KEYWORD", "float");
    }

    bool isFunctionStart() const {
        return isTypeKeyword() &&
            pos + 1 < tokens.size() &&
            tokens[pos + 1].type == "IDENTIFIER" &&
            tokens[pos + 1].value == "main";
    }

    bool isRelOp() const {
        if (!check("OPERATOR")) return false;
        string op = current().value;
        return op == "<" || op == ">" || op == "==" || op == "!=" || op == "<=" || op == ">=";
    }

    void synchronizeStatement() {
        while (!check("EOF") && !check("DELIMITER", ";") && !check("DELIMITER", "}")) {
            pos++;
        }
        if (check("DELIMITER", ";")) pos++;
    }

public:
    Parser(const vector<Token>& input) : tokens(input), pos(0) {}

    vector<ParserError> getErrors() const {
        return errors;
    }

    shared_ptr<ASTNode> parseProgram() {
        auto root = make_shared<ASTNode>("Program");

        if (check("KEYWORD", "using")) {
            root->add(parseUsingNamespace());
        }

        while (!check("EOF") && !isFunctionStart()) {
            addError("начало функции main: int main() или float main()");
            skipBadToken();
        }

        if (!check("EOF")) {
            root->add(parseFunction());
        }

        if (!check("EOF")) {
            addError("конец файла");
        }

        return root;
    }

    shared_ptr<ASTNode> parseUsingNamespace() {
        auto node = make_shared<ASTNode>("UsingNamespace");

        expect("KEYWORD", "using", "ключевое слово using");
        expect("KEYWORD", "namespace", "ключевое слово namespace");

        if (check("IDENTIFIER")) {
            node->add(make_shared<ASTNode>("name", current().value));
            pos++;
        }
        else {
            addError("имя пространства имен");
        }

        expect("DELIMITER", ";", "разделитель ;");
        return node;
    }

    shared_ptr<ASTNode> parseFunction() {
        auto node = make_shared<ASTNode>("Function", "main");

        if (isTypeKeyword()) {
            node->add(make_shared<ASTNode>("return_type", current().value));
            pos++;
        }
        else {
            addError("тип возвращаемого значения int или float");
        }

        expect("IDENTIFIER", "main", "идентификатор main");
        expect("DELIMITER", "(", "открывающая скобка (");
        expect("DELIMITER", ")", "закрывающая скобка )");

        node->add(parseBlock());
        return node;
    }

    shared_ptr<ASTNode> parseBlock() {
        auto node = make_shared<ASTNode>("Block");

        expect("DELIMITER", "{", "открывающая операторная скобка {");

        while (!check("EOF") && !check("DELIMITER", "}")) {
            node->add(parseStatement());
        }

        expect("DELIMITER", "}", "закрывающая операторная скобка }");
        return node;
    }

    shared_ptr<ASTNode> parseStatement() {
        if (check("ERROR") || check("UNKNOWN")) {
            auto node = make_shared<ASTNode>("ErrorStatement", current().value);
            addError("корректный токен из потока ЛР2");
            skipBadToken();
            return node;
        }

        if (isTypeKeyword()) return parseVarDecl();
        if (check("IDENTIFIER", "cout")) return parseCoutStmt();
        if (check("IDENTIFIER")) return parseAssignStmt();
        if (check("KEYWORD", "if")) return parseIfStmt();

        auto node = make_shared<ASTNode>("ErrorStatement", current().value);
        addError("объявление переменной, присваивание, cout или if");
        synchronizeStatement();
        return node;
    }

    shared_ptr<ASTNode> parseVarDecl() {
        auto node = make_shared<ASTNode>("VarDecl");

        node->add(make_shared<ASTNode>("type", current().value));
        pos++;

        if (check("IDENTIFIER")) {
            node->add(make_shared<ASTNode>("name", current().value));
            pos++;
        }
        else {
            addError("имя переменной");
        }

        expect("DELIMITER", ";", "разделитель ; после объявления переменной");
        return node;
    }

    shared_ptr<ASTNode> parseAssignStmt() {
        auto node = make_shared<ASTNode>("AssignStmt");

        node->add(make_shared<ASTNode>("left", current().value));
        pos++;

        expect("OPERATOR", "=", "оператор присваивания =");
        node->add(parseExpression());
        expect("DELIMITER", ";", "разделитель ; после присваивания");

        return node;
    }

    shared_ptr<ASTNode> parseExpression() {
        auto left = parseOperand();

        if (check("OPERATOR") &&
            (current().value == "+" || current().value == "-" || current().value == "*" || current().value == "/")) {
            auto node = make_shared<ASTNode>("BinaryExpr", current().value);
            pos++;
            node->add(left);
            node->add(parseOperand());
            return node;
        }

        return left;
    }

    shared_ptr<ASTNode> parseOperand() {
        if (check("IDENTIFIER")) {
            string value = current().value;
            pos++;
            return make_shared<ASTNode>("Identifier", value);
        }

        if (check("CONSTANT_INT")) {
            string value = current().value;
            pos++;
            return make_shared<ASTNode>("IntConst", value);
        }

        if (check("CONSTANT_FLOAT")) {
            string value = current().value;
            pos++;
            return make_shared<ASTNode>("FloatConst", value);
        }

        if (check("STRING")) {
            string value = current().value;
            pos++;
            return make_shared<ASTNode>("StringConst", value);
        }

        if (check("ERROR") || check("UNKNOWN")) {
            string value = current().value;
            addError("корректный операнд, а не ошибочный токен из ЛР2");
            skipBadToken();
            return make_shared<ASTNode>("ErrorOperand", value);
        }

        addError("идентификатор, целая, вещественная или строковая константа");
        skipBadToken();
        return make_shared<ASTNode>("ErrorOperand");
    }

    shared_ptr<ASTNode> parseCondition() {
        auto node = make_shared<ASTNode>("Condition");

        node->add(parseOperand());

        if (isRelOp()) {
            node->add(make_shared<ASTNode>("operator", current().value));
            pos++;
        }
        else {
            addError("оператор сравнения <, >, ==, !=, <= или >=");
        }

        node->add(parseOperand());
        return node;
    }

    shared_ptr<ASTNode> parseIfStmt() {
        auto node = make_shared<ASTNode>("IfStmt");

        expect("KEYWORD", "if", "ключевое слово if");
        expect("DELIMITER", "(", "открывающая скобка условия (");
        node->add(parseCondition());
        expect("DELIMITER", ")", "закрывающая скобка условия )");
        node->add(parseBlock());

        if (match("KEYWORD", "else")) {
            auto elseNode = make_shared<ASTNode>("ElsePart");

            if (check("KEYWORD", "if")) {
                elseNode->add(parseIfStmt());
            }
            else {
                elseNode->add(parseBlock());
            }

            node->add(elseNode);
        }

        return node;
    }

    shared_ptr<ASTNode> parseCoutStmt() {
        auto node = make_shared<ASTNode>("CoutStmt");

        expect("IDENTIFIER", "cout", "идентификатор cout");

        do {
            expect("OPERATOR", "<<", "оператор вывода <<");
            node->add(parseOperand());
        } while (check("OPERATOR", "<<"));

        expect("DELIMITER", ";", "разделитель ; после оператора вывода");
        return node;
    }
};

void printTokens(const vector<Token>& tokens) {
    cout << "Поток токенов из файла test.txt:\n";
    for (const Token& t : tokens) {
        if (t.type == "EOF") continue;
        cout << t.index << ". (" << t.type << ", " << t.value << ")\n";
    }
}

void printAST(const shared_ptr<ASTNode>& node, const string& prefix = "", bool isLast = true) {
    if (!node) return;

    cout << prefix << (isLast ? "`-- " : "|-- ");
    cout << node->name;
    if (!node->value.empty()) cout << ": " << node->value;
    cout << endl;

    for (size_t i = 0; i < node->children.size(); i++) {
        bool childIsLast = (i + 1 == node->children.size());
        printAST(node->children[i], prefix + (isLast ? "    " : "|   "), childIsLast);
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    vector<string> formatErrors;
    vector<Token> tokens = readTokensFromFile("test.txt", formatErrors);

    if (!formatErrors.empty()) {
        cout << "Ошибки формата файла test.txt:\n";
        for (const string& e : formatErrors) {
            cout << "- " << e << endl;
        }
        return 1;
    }

    printTokens(tokens);

    Parser parser(tokens);
    shared_ptr<ASTNode> ast = parser.parseProgram();
    vector<ParserError> parserErrors = parser.getErrors();

    cout << "\nAST:\n";
    printAST(ast);

    cout << "\nРезультат синтаксического анализа:\n";
    if (parserErrors.empty()) {
        cout << "Синтаксический анализ завершен успешно. Ошибок не найдено.\n";
    }
    else {
        cout << "Обнаружены ошибки:\n";
        for (const ParserError& e : parserErrors) {
            cout << "Синтаксическая ошибка в токене " << e.tokenIndex
                << " ('" << e.tokenValue << "'): " << e.message << endl;
        }
    }

    return 0;
}