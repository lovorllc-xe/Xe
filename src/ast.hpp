#pragma once
#include <memory>
#include <string>
#include <vector>
namespace xe {
struct Expr { virtual ~Expr() = default; };
using E = std::shared_ptr<Expr>;
struct Literal : Expr { std::string value; bool string = false; };
struct Name : Expr { std::string value; };
struct Unary : Expr { std::string op; E operand; };
struct Binary : Expr { std::string op; E left, right; };
struct Call : Expr { std::string name; std::vector<E> args; };
struct Index : Expr { E base, index; };
struct Slice : Expr { E base, start, finish; };
struct Member : Expr { E base; std::string name; };

struct Stmt { virtual ~Stmt() = default; int line = 0, column = 0; };
using S = std::shared_ptr<Stmt>;
struct Block : Stmt { std::vector<S> items; };
struct Var : Stmt { std::string name, type; E value; bool immutable = false; };
struct Assign : Stmt { E target, value; };
struct ExprStmt : Stmt { E value; };
struct Defer : Stmt { E action; };
struct Return : Stmt { E value; };
struct Gc : Stmt { std::shared_ptr<Block> body; };
struct Asm : Stmt { std::string text; };
struct If : Stmt { E condition; std::shared_ptr<Block> yes, no; };
struct While : Stmt { E condition; std::shared_ptr<Block> body; };
struct SwitchCase { E value; std::shared_ptr<Block> body; };
struct Switch : Stmt { E value; std::vector<SwitchCase> cases; std::shared_ptr<Block> fallback; };

struct Param { std::string name, type; int line = 0, column = 0; };
struct Function { std::string name, result; std::vector<Param> params; std::shared_ptr<Block> body; int line = 0, column = 0; };
struct TypeAlias { std::string name, target; int line = 0, column = 0; };
struct RecordField { std::string name, type; int line = 0, column = 0; };
struct RecordType { std::string name; std::vector<RecordField> fields; int line = 0, column = 0; };
struct Program {
    std::vector<Function> functions;
    std::vector<TypeAlias> aliases;
    std::vector<RecordType> records;
    std::vector<std::string> imports;
};

using FunctionDeclNode = Function;
using BlockNode = Block;
using GcBlockNode = Gc;
using AsmBlockNode = Asm;
}
