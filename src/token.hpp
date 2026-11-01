#pragma once
#include <string>
#include <vector>
namespace xe {
enum class TokenType {
    Identifier, Number, String, RawAsm, Newline, End,
    Colon, Comma, Dot, LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Less, Greater, Plus, Minus, Star, Slash, Percent, Amp, At, Hash, Bang,
    Assign, Equal, NotEqual, LessEqual, GreaterEqual, AndAnd, OrOr,
    ShiftLeft, ShiftRight, BitXor, BitNot, BitOr
};
struct Token { TokenType type; std::string text; int line; int column; };
}
