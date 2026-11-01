#include "parser.hpp"
#include <stdexcept>
namespace xe {
Parser::Parser(std::vector<Token> input, std::unordered_map<std::string, std::string> sharedAliases, bool allowLegacyConditions)
    : tokens(std::move(input)), aliases(std::move(sharedAliases)), allowLegacyConditions(allowLegacyConditions) {}
const Token& Parser::current() const { return tokens[pos]; }
bool Parser::is(TokenType type) const { return current().type == type; }
bool Parser::word(const std::string& value) const { return is(TokenType::Identifier) && current().text == value; }
Token Parser::take() { return tokens[pos++]; }
Token Parser::expect(TokenType type, const char* description) {
    if (!is(type)) throw std::runtime_error("line " + std::to_string(current().line) + ", column " + std::to_string(current().column) + ": expected " + description);
    return take();
}
Token Parser::expectWord(const char* value) {
    if (!word(value)) throw std::runtime_error("line " + std::to_string(current().line) + ", column " + std::to_string(current().column) + ": expected '" + value + "'");
    return take();
}
void Parser::newlines() { while (is(TokenType::Newline)) take(); }
std::string Parser::path() {
    std::string result = expect(TokenType::Identifier, "name").text;
    while (is(TokenType::Dot)) { take(); result += "." + expect(TokenType::Identifier, "name after '.'").text; }
    return result;
}
std::string Parser::type() {
    if (is(TokenType::Star)) { take(); return "*" + type(); }
    if (is(TokenType::LBracket)) {
        take(); auto count = expect(TokenType::Number, "array length").text;
        expect(TokenType::RBracket, "']'"); return "[" + count + "]" + type();
    }
    std::string result = expect(TokenType::Identifier, "type").text;
    while (is(TokenType::Dot)) { take(); result += "." + expect(TokenType::Identifier, "type name after '.'").text; }
    if (!currentModule.empty()) if (auto alias = aliases.find(currentModule + result); alias != aliases.end()) return alias->second;
    if (auto alias = aliases.find(result); alias != aliases.end()) return alias->second;
    return result;
}
Function Parser::function(const std::string& prefix) {
    auto declaration = expectWord("fn"); Function result; result.line = declaration.line; result.column = declaration.column; result.name = prefix + path(); expect(TokenType::LParen, "'('");
    if (!is(TokenType::RParen)) {
        do {
            Param param; param.line = current().line; param.column = current().column; param.name = path(); expect(TokenType::Colon, "':'"); param.type = type(); result.params.push_back(std::move(param));
            if (!is(TokenType::Comma)) break;
            take();
        } while (true);
    }
    expect(TokenType::RParen, "')'");
    if (is(TokenType::Colon)) { take(); result.result = type(); } else result.result = "void";
    newlines(); expectWord("begin"); result.body = block(true); return result;
}
TypeAlias Parser::typeAlias(const std::string& prefix) {
    auto declaration = expectWord("type"); TypeAlias result; result.line = declaration.line; result.column = declaration.column; result.name = path();
    if (!prefix.empty() && result.name.find('.') == std::string::npos) result.name = prefix + result.name;
    expect(TokenType::Colon, "':'"); result.target = type(); aliases[result.name] = result.target; return result;
}
RecordType Parser::recordType(const std::string& prefix) {
    auto declaration = expectWord("type"); RecordType result; result.line = declaration.line; result.column = declaration.column;
    result.name = path(); if (!prefix.empty() && result.name.find('.') == std::string::npos) result.name = prefix + result.name;
    expect(TokenType::Colon, "':'"); expectWord("record"); newlines(); expectWord("begin"); newlines();
    while (!is(TokenType::End) && !word("end")) {
        RecordField field; field.line = current().line; field.column = current().column; field.name = path();
        expect(TokenType::Colon, "':' after record field"); field.type = type();
        result.fields.push_back(std::move(field));
        if (!is(TokenType::Newline) && !is(TokenType::End) && !word("end"))
            throw std::runtime_error("line " + std::to_string(current().line) + ", column " + std::to_string(current().column) + ": expected end of record field");
        newlines();
    }
    expectWord("end"); return result;
}
Program Parser::parse() {
    Program program; newlines();
    while (!is(TokenType::End)) {
        if (word("uses")) {
            take();
            do {
                program.imports.push_back(path());
                if (!is(TokenType::Comma)) break;
                take();
            } while (true);
            if (!is(TokenType::Newline) && !is(TokenType::End))
                throw std::runtime_error("line " + std::to_string(current().line) + ", column " + std::to_string(current().column) + ": expected end of uses declaration");
            newlines();
        } else if (word("module")) {
            take(); std::string moduleName = path(); newlines(); expectWord("begin"); newlines();
            std::string savedModule = currentModule; currentModule = moduleName + ".";
            while (!is(TokenType::End) && !word("end")) {
                if (word("type") && pos + 3 < tokens.size() && tokens[pos + 3].type == TokenType::Identifier && tokens[pos + 3].text == "record") program.records.push_back(recordType(currentModule));
                else if (word("type")) program.aliases.push_back(typeAlias(currentModule));
                else program.functions.push_back(function(currentModule));
                newlines();
            }
            expectWord("end"); currentModule = std::move(savedModule); newlines();
        } else if (word("type") && pos + 3 < tokens.size() && tokens[pos + 3].type == TokenType::Identifier && tokens[pos + 3].text == "record") { program.records.push_back(recordType("")); newlines(); }
        else if (word("type")) { program.aliases.push_back(typeAlias("")); newlines(); }
        else { program.functions.push_back(function("")); newlines(); }
    }
    return program;
}
std::shared_ptr<Block> Parser::block(bool heavy) {
    auto result = std::make_shared<Block>(); newlines();
    while (!is(TokenType::End) && !(heavy && word("end")) && !(!heavy && is(TokenType::RBrace))) {
        result->items.push_back(statement());
        if (!is(TokenType::Newline) && !is(TokenType::End) && !(heavy && word("end")) && !(!heavy && is(TokenType::RBrace)))
            throw std::runtime_error("line " + std::to_string(current().line) + ", column " + std::to_string(current().column) + ": expected end of statement");
        newlines();
    }
    if (heavy) expectWord("end"); else expect(TokenType::RBrace, "'}'");
    return result;
}
S Parser::statement() {
    const Token start = current();
    auto locate = [&](const S& node) { node->line = start.line; node->column = start.column; return node; };
    if (word("var") || word("const")) {
        auto declaration = take(); auto node = std::make_shared<Var>(); node->immutable = declaration.text == "const"; node->name = path(); expect(TokenType::Colon, "':'"); node->type = type();
        if (is(TokenType::Assign)) { take(); node->value = expression(); }
        if (node->immutable && !node->value) throw std::runtime_error("const declaration requires an initializer");
        return locate(node);
    }
    if (word("defer")) {
        take(); auto node = std::make_shared<Defer>(); node->action = expression(); return locate(node);
    }
    if (word("return")) { take(); auto node = std::make_shared<Return>(); if (!is(TokenType::Newline) && !word("end") && !is(TokenType::RBrace) && !is(TokenType::End)) node->value = expression(); return locate(node); }
    if (word("gc")) {
        take(); expect(TokenType::Dot, "'.'"); expectWord("begin"); newlines(); auto node = std::make_shared<Gc>();
        if (is(TokenType::LBrace)) { take(); node->body = block(false); newlines(); expectWord("gc"); expect(TokenType::Dot, "'.'"); expectWord("end"); }
        else {
            node->body = std::make_shared<Block>();
            while (!is(TokenType::End) && !(word("gc") && pos + 2 < tokens.size() && tokens[pos + 1].type == TokenType::Dot && tokens[pos + 2].text == "end")) {
                if (is(TokenType::Newline)) { newlines(); continue; }
                node->body->items.push_back(statement());
                if (is(TokenType::Newline)) newlines();
            }
            expectWord("gc"); expect(TokenType::Dot, "'.'"); expectWord("end");
        }
        return locate(node);
    }
    if (word("if")) {
        take(); auto node = std::make_shared<If>();
        if (allowLegacyConditions) {
            node->condition = expression();
        } else {
            expect(TokenType::LParen, "'(' before if condition");
            node->condition = expression();
            expect(TokenType::RParen, "')' after if condition");
        }
        newlines(); expect(TokenType::LBrace, "'{' after condition"); node->yes = block(false);
        auto saved = pos; newlines(); if (word("else")) { take(); newlines(); expect(TokenType::LBrace, "'{' after else"); node->no = block(false); } else pos = saved;
        return locate(node);
    }
    if (word("for")) {
        take(); auto node = std::make_shared<While>();
        if (allowLegacyConditions) {
            node->condition = expression();
        } else {
            expect(TokenType::LParen, "'(' before for condition");
            node->condition = expression();
            expect(TokenType::RParen, "')' after for condition");
        }
        newlines(); expect(TokenType::LBrace, "'{' after for condition"); node->body = block(false);
        return locate(node);
    }
    if (word("while")) { take(); auto node = std::make_shared<While>(); node->condition = expression(); newlines(); expect(TokenType::LBrace, "'{' after condition"); node->body = block(false); return locate(node); }
    if (word("switch")) {
        take(); auto node = std::make_shared<Switch>(); node->value = expression(); newlines(); expectWord("begin"); newlines();
        while (!is(TokenType::End) && !word("end")) {
            bool fallback = word("default"); if (fallback) take(); else expectWord("case");
            E label; if (!fallback) label = expression(); newlines(); auto arm = std::make_shared<Block>();
            while (!is(TokenType::End) && !word("end") && !word("default") && !word("case")) { arm->items.push_back(statement()); newlines(); }
            if (fallback) { if (node->fallback) throw std::runtime_error("switch has more than one default arm"); node->fallback = arm; }
            else node->cases.push_back({std::move(label), std::move(arm)});
        }
        expectWord("end"); return locate(node);
    }
    if (is(TokenType::RawAsm)) { auto node = std::make_shared<Asm>(); node->text = take().text; return locate(node); }
    E left = expression();
    if (is(TokenType::Assign)) { take(); auto node = std::make_shared<Assign>(); node->target = std::move(left); node->value = expression(); return locate(node); }
    auto node = std::make_shared<ExprStmt>(); node->value = std::move(left); return locate(node);
}
int Parser::precedence(TokenType type) const {
    switch (type) {
        case TokenType::OrOr: return 1; case TokenType::AndAnd: return 2; case TokenType::BitOr: return 3;
        case TokenType::BitXor: return 4; case TokenType::Amp: return 5;
        case TokenType::Equal: case TokenType::NotEqual: return 6;
        case TokenType::Less: case TokenType::Greater: case TokenType::LessEqual: case TokenType::GreaterEqual: return 7;
        case TokenType::ShiftLeft: case TokenType::ShiftRight: return 8;
        case TokenType::Plus: case TokenType::Minus: return 9;
        case TokenType::Star: case TokenType::Slash: case TokenType::Percent: return 10;
        default: return 0;
    }
}
E Parser::expression(int minimum) {
    E left;
    if (is(TokenType::Minus) || is(TokenType::Bang) || is(TokenType::BitNot) || is(TokenType::Star) || is(TokenType::Amp) || is(TokenType::At) || is(TokenType::Hash)) {
        auto unary = std::make_shared<Unary>(); unary->op = take().text; unary->operand = expression(11); left = unary;
    } else if (is(TokenType::Number) || is(TokenType::String)) {
        auto token = take(); auto literal = std::make_shared<Literal>(); literal->value = token.text; literal->string = token.type == TokenType::String; left = literal;
    } else if (is(TokenType::LParen)) { take(); left = expression(); expect(TokenType::RParen, "')'"); }
    else {
        auto name = path();
        if (is(TokenType::LParen)) {
            take(); auto call = std::make_shared<Call>(); call->name = std::move(name);
            if (!currentModule.empty() && call->name.find('.') == std::string::npos && call->name != "write" && call->name != "writeln" && call->name != "read" && call->name != "readln" && call->name != "alloc" && call->name != "new" && call->name != "alloc.aligned" && call->name != "aligned_alloc" && call->name != "free" && call->name != "delete") call->name = currentModule + call->name;
            if (!is(TokenType::RParen)) { do { call->args.push_back(expression()); if (!is(TokenType::Comma)) break; take(); } while (true); }
            expect(TokenType::RParen, "')'"); left = call;
        } else {
            auto first = name.find('.');
            if (first == std::string::npos) { auto value = std::make_shared<Name>(); value->value = std::move(name); left = value; }
            else {
                auto value = std::make_shared<Name>(); value->value = name.substr(0, first); left = value;
                std::size_t start = first + 1;
                while (start <= name.size()) {
                    auto finish = name.find('.', start); if (finish == std::string::npos) finish = name.size();
                    auto member = std::make_shared<Member>(); member->base = left; member->name = name.substr(start, finish - start); left = member;
                    if (finish == name.size()) break;
                    start = finish + 1;
                }
            }
        }
    }
    while (true) {
        if (is(TokenType::Dot)) {
            take(); auto member = std::make_shared<Member>(); member->base = left; member->name = expect(TokenType::Identifier, "field name after '.'").text; left = member; continue;
        }
        if (is(TokenType::LBracket)) {
            take(); E first; if (!is(TokenType::Colon)) first = expression();
            if (is(TokenType::Colon)) { take(); E finish; if (!is(TokenType::RBracket)) finish = expression(); expect(TokenType::RBracket, "']'"); auto value = std::make_shared<Slice>(); value->base = left; value->start = first; value->finish = finish; left = value; }
            else { if (!first) throw std::runtime_error("empty array index"); expect(TokenType::RBracket, "']'"); auto value = std::make_shared<Index>(); value->base = left; value->index = first; left = value; }
            continue;
        }
        int p = precedence(current().type); if (p < minimum || p == 0) break;
        auto op = take(); auto right = expression(p + 1); auto value = std::make_shared<Binary>(); value->op = op.text; value->left = left; value->right = right; left = value;
    }
    return left;
}
}
