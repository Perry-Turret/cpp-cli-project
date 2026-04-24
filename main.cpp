#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <filesystem>
#include <chrono>
#include <thread>

using std::cout;
using std::string;
using std::vector;
namespace fs = std::filesystem;

enum TOKEN_TYPE {
    MAKE,
    PROJECT,
    BUILD,
    IN,
    OUT,
    IDENT,
    STRING,
    PATH,
    END,
    WITH,
    LPAREN,
    RPAREN,
    COMMA,
};

struct Token{
    TOKEN_TYPE type;
    string value;
    Token(TOKEN_TYPE t, const string& v)
        : type(t), value(v){}
};

Token ret_tok(string &in){
    if (in == "make") return Token(MAKE, "make");
    else if (in == "build") return Token(BUILD, "build");
    else if (in == "in") return Token(IN, "in");
    else if (in == "out") return Token(OUT, "out");
    else if (in == "with") return Token(WITH, "with");
    else if (in == "(") return Token(LPAREN, "(");
    else if (in == ")") return Token(RPAREN, ")");
    else if (in.empty()) return Token(END, "EOF");
    else if (in == ",") return Token(COMMA, ",");
    else return Token(IDENT, in);
};

bool isKeyword(TOKEN_TYPE t) {
    TOKEN_TYPE keywords[] = {MAKE, BUILD, PROJECT, IN, OUT};

    return std::find(std::begin(keywords), std::end(keywords), t) != std::end(keywords);
}

vector<Token> tokenize(const string& input){
    vector<Token> tokens;
    string current;
    for (size_t i = 0;i<input.size();i++){
        if (input[i] == ' '){
            if (!current.empty()){
                tokens.push_back(ret_tok(current));
                current.clear();
            }
        }else if(input[i] == '"'){
            string rts;
            i++;
            while (i < input.size() && input[i] != '"'){
                rts += input[i];
                i++;
            }
            if (i >= input.size()) {
                cout << "Error: missing closing quote\n";
                break;
            }
            tokens.push_back(Token(STRING, rts));
        }else{
            current += input[i];
        }
    }
    if (!current.empty()){
        tokens.push_back(ret_tok(current));
    }
    tokens.push_back(Token(END, "EOF"));
    return tokens;
}

struct Command{
    TOKEN_TYPE type;
    string name;
    string path;
    bool unsafe_optim = false;
    bool fast_optim = false;
    bool high_optim = false;
    vector<string> links;
    string output;
};

class Parser {
public:
    Parser(const vector<Token>& t)
        :   tokens(t), pos(0) {}
    vector<Command> parseProgram(){
        vector<Command> commands;
        while (current().type != END){
            if (current().type == MAKE){
                commands.push_back(parseMake());
            }
            else if (current().type == BUILD){
                commands.push_back(parseBuild());
            }
            else{
                pos++;
            }
        }
        return commands;
    }
private:
    vector<Token> tokens;
    size_t pos;

    Token current(){
        if (pos >= tokens.size()) return Token(END, "EOF");
        return tokens[pos];
    }

    void eat(TOKEN_TYPE type){
        if (current().type == type){ pos++;}else{cout << "Expected: " << type << " but got: " << current().type << "\n";}
    }

    Command parseMake(){
        Command cmd{};
        eat(MAKE);
        cmd.name = current().value; eat(STRING);
        eat(IN);
        cmd.path = current().value; eat(STRING);
        cmd.type = MAKE;
        return cmd; 
    }

    Command parseBuild(){
        Command cmd{};
        eat(BUILD);
        cmd.name = current().value; eat(STRING);
        if (current().type == IN){
            eat(IN);
            cmd.path = current().value; eat(STRING);
        }
        if (current().type == WITH){
            eat(WITH);
            if (current().type == LPAREN){
                eat(LPAREN);
                while (current().type != RPAREN){
                    cmd.links.push_back(current().value);
                    eat(STRING);
                    if (current().type == COMMA) {
                        eat(COMMA);
                    } else if (current().type != RPAREN) {
                        cout << "Expected ',' or ')' in WITH list, but got: " << current().type << "\n";
                        break;
                    }
                }
                eat(RPAREN);
            }
        }
        if (current().type == OUT){
            eat(OUT);
            cmd.output = current().value; eat(STRING);
        }
        while (current().type == IDENT){
            if (current().value == "unsafe_optim") cmd.unsafe_optim = true;
            else if (current().value == "high_optim") cmd.high_optim = true;
            else if (current().value == "fast_optim") cmd.fast_optim = true;
            eat(IDENT);
        }
        cmd.type = BUILD;
        return cmd;
    }
};

std::ostream& operator<<(std::ostream& os, const Command& cmd) {
    os << "Command{type: " << cmd.type 
       << ", name: " << cmd.name 
       << ", path: " << cmd.path 
       << ", output: " << cmd.output << "}";
    os << " Links: ";
    for (const auto& l : cmd.links) os << l << " ";
    return os;
}

void make_dir(const Command& cmd, const string& place){
    fs::path dir = place;
    if (cmd.path.empty()) {
        dir = fs::current_path();
    }
    if (fs::exists(dir)){
        cout << "Found: " << place << "\n";
    }else{
        for (int i = 0; i < 6; i++) {
            cout << "\rCreating directory";
            for (int j = 0; j < (i % 4); j++) {
                cout << ".";
            }
            cout << "   " << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        fs::create_directories(dir);
    }
    fs::path file = dir / (cmd.name + ".cpp");
    if (fs::exists(file)) {
        cout << "File already exists: " << file << "\n";
        return;
    }else{
        std::ofstream f(file);
        f << "#include <iostream>\n\nint main(){\n    //thanks for using my app!\n  std::cout << \"Hello, World!\";\n}";
        f.close();
    }
}

int main(){
    string input;
    cout << "> ";
    getline(std::cin, input);
    cout << '\n';
    vector<Token> tokens = tokenize(input);
    Parser p{tokens};
    vector<Command> cmds = p.parseProgram();
    for (const auto &cmd : cmds){
        if (cmd.type == MAKE){
            make_dir(cmd, cmd.path);
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    return 0;
}