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

vector<Token> tokenize(const string& in){
    vector<Token> tokens;

    for (size_t i = 0; i < in.size();){
        
        if (isspace(in[i])){
            i++;
        }

        else if (in[i] == '"'){
            i++;
            string val;

            while (i < in.size() && in[i] != '"'){
                val += in[i];
                i++;
            }

            tokens.push_back(Token(STRING, val));

            if (i < in.size()) i++;
        }else if (in[i] == '('){
            tokens.push_back(Token(LPAREN, "("));
            i++;
        }else if (in[i] == ')'){
            tokens.push_back(Token(RPAREN, ")"));
            i++;
        }else if (in[i] == ','){
            tokens.push_back(Token(COMMA, ","));
            i++;
        }else if (isalpha(in[i])){
            string word;
            while (i < in.size() &&
                  (isalnum(in[i]) || in[i] == '_')){
                word += in[i];
                i++;
            }
            tokens.push_back(ret_tok(word));
        }else {
            cout << "Unknown char (IDK what the hell this is): " << in[i] << "\n";
            i++;
        }
    }
    return tokens;
}

struct Command{
    TOKEN_TYPE type;
    string name;
    string path;
    bool low_data = false;
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

    void debug_tok() {
        for (size_t i = 0; i < tokens.size(); i++) {
            cout << "[" << i << "] "
                 << "type=" << tokens[i].type
                 << " value=" << tokens[i].value
                 << "\n";
        }
    }

private:
    vector<Token> tokens;
    size_t pos;

    Token current(){
        if (pos >= tokens.size()) return Token(END, "EOF");
        return tokens[pos];
    }

    void eat(TOKEN_TYPE type){
        if (current().type != type){
            cout << "Parse error: expected " << type
                << " got " << current().type << "\n";
            return;
        }
        pos++;
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
            eat(LPAREN);
            eat(WITH);
            eat(LPAREN);
            while (current().type != RPAREN && current().type != END){
                if (current().type != STRING){
                    cout << "Expected string in WITH list\n";
                    break;
                }
                cmd.links.push_back(current().value);
                eat(STRING);
                if (current().type == COMMA){
                    eat(COMMA);
                }
            }
            eat(RPAREN);
            eat(RPAREN);
        }
        if (current().type == OUT){
            eat(OUT);
            cmd.output = current().value; eat(STRING);
        }
        while (current().type == IDENT){
            if (current().value == "low_data") cmd.low_data = true;
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
        for (int i = 0; i < 10; i++) {
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
    file = dir / ("README.md");
    if (fs::exists(file)) {
        cout << "File already exists: " << file << "\n";
        return;
    }else{
        std::ofstream f(file);
        f << "# file\n"
         << "Generated by MyTool\n\n"
         << "## How to run\n"
         << "Compile with:\n"
         << "g++ " << cmd.name << ".cpp -o " << cmd.name << "\n"
         << "./" << cmd.name << "\n\n"
         << "## Notes\n"
         << "- This is a starter project\n"
         << "- Modify freely\n";
        f.close();
    }
}

string build_cmd(Command &cmd){
    string tags;
    if (cmd.unsafe_optim){
        tags = "-Ofast";
    }else if (cmd.fast_optim){
        tags = "-O3";
    }else if (cmd.high_optim){
        tags = "-O2";
    }else{
        tags = "-O0";
    }if (cmd.low_data){
        tags += " -s";
    }
    string sf = "\"" + cmd.path + "/" + cmd.name + ".cpp\"";
    cout << "g++ " << sf << " " << tags << " -o " + '\"' + cmd.output + "\"";
    return "g++ " + sf + " " + tags + " -o " + "\"" + cmd.output + "\"" ;
}

int main(){
    string input;
    cout << "> ";
    getline(std::cin, input);
    cout << '\n';
    vector<Token> tokens = tokenize(input);
    Parser p{tokens};
    vector<Command> cmds = p.parseProgram();
    for (auto &cmd : cmds){
        if (cmd.type == MAKE){
            make_dir(cmd, cmd.path);
        }if (cmd.type == BUILD){
            string c = build_cmd(cmd);
            system(c.c_str());
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    return 0;
}