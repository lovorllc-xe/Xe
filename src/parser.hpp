#pragma once
#include "ast.hpp"
#include "token.hpp"
#include <unordered_map>
namespace xe {
class Parser {
public:
    explicit Parser(std::vector<Token> tokens, std::unordered_map<std::string, std::string> sharedAliases = {});
    Program parse();
private:
    std::vector<Token> tokens;
    std::size_t pos = 0;
    std::string currentModule;
    std::unordered_map<std::string, std::string> aliases;
    const Token& current() const;
    bool is(TokenType type) const;
    bool word(const std::string& value) const;
    Token take();
    Token expect(TokenType type, const char* description);
    Token expectWord(const char* value);
    void newlines();
    std::string path();
    std::string type();
    Function function(const std::string& prefix);
    TypeAlias typeAlias(const std::string& prefix);
    RecordType recordType(const std::string& prefix);
    std::shared_ptr<Block> block(bool heavy);
    S statement();
    E expression(int minimum = 1);
    E prefix();
    int precedence(TokenType type) const;
};
}
