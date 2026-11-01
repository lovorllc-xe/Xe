#pragma once
#include "token.hpp"
#include <string_view>
namespace xe {
class Lexer {
public:
    explicit Lexer(std::string_view source);
    std::vector<Token> scan();
private:
    std::string_view source;
    std::size_t pos = 0;
    int line = 1, column = 1;
    bool continuation = false;
    char peek(std::size_t n = 0) const;
    char take();
    void emit(std::vector<Token>&, TokenType, std::string, int, int);
    std::string readAsm(int startLine, int startColumn);
};
}
