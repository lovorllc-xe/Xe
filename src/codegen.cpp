#include "codegen.hpp"
#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
namespace xe {
namespace {
std::string switchLabel(const E& expression) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expression)) return "std::int64_t{" + std::to_string(std::stoll(literal->value)) + "}";
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "-") {
        if (auto literal = std::dynamic_pointer_cast<Literal>(unary->operand)) {
            if (std::stoull(literal->value) == static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1) return "std::numeric_limits<std::int64_t>::min()";
            return "std::int64_t{-" + std::to_string(std::stoll(literal->value)) + "}";
        }
    }
    throw std::runtime_error("switch case is not an integer constant");
}
bool expressionReferencesName(const E& expression, const std::string& wanted) {
    if (!expression) return false;
    if (auto value = std::dynamic_pointer_cast<Name>(expression)) return value->value == wanted;
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) return expressionReferencesName(unary->operand, wanted);
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) return expressionReferencesName(binary->left, wanted) || expressionReferencesName(binary->right, wanted);
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        for (const auto& argument : call->args) if (expressionReferencesName(argument, wanted)) return true;
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) return expressionReferencesName(index->base, wanted) || expressionReferencesName(index->index, wanted);
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) return expressionReferencesName(slice->base, wanted) || expressionReferencesName(slice->start, wanted) || expressionReferencesName(slice->finish, wanted);
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) return expressionReferencesName(member->base, wanted);
    return false;
}
bool expressionMutatesName(const E& expression, const std::string& wanted) {
    if (!expression) return false;
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) {
        if ((unary->op == "@" || unary->op == "&") && expressionReferencesName(unary->operand, wanted)) return true;
        return expressionMutatesName(unary->operand, wanted);
    }
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) return expressionMutatesName(binary->left, wanted) || expressionMutatesName(binary->right, wanted);
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        if (call->name == "read" || call->name == "readln") {
            for (const auto& argument : call->args) if (expressionReferencesName(argument, wanted)) return true;
        }
        for (const auto& argument : call->args) if (expressionMutatesName(argument, wanted)) return true;
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) return expressionMutatesName(index->base, wanted) || expressionMutatesName(index->index, wanted);
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) return expressionMutatesName(slice->base, wanted) || expressionMutatesName(slice->start, wanted) || expressionMutatesName(slice->finish, wanted);
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) return expressionMutatesName(member->base, wanted);
    return false;
}
bool assignmentRootIsName(const E& expression, const std::string& wanted) {
    if (auto value = std::dynamic_pointer_cast<Name>(expression)) return value->value == wanted;
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) return assignmentRootIsName(member->base, wanted);
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) return assignmentRootIsName(index->base, wanted);
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "*") return assignmentRootIsName(unary->operand, wanted);
    return false;
}
bool statementMutatesName(const S& statement, const std::string& wanted) {
    if (!statement) return false;
    if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) return assignmentRootIsName(assignment->target, wanted) || expressionMutatesName(assignment->target, wanted) || expressionMutatesName(assignment->value, wanted);
    if (auto variable = std::dynamic_pointer_cast<Var>(statement)) return expressionMutatesName(variable->value, wanted);
    if (auto expression = std::dynamic_pointer_cast<ExprStmt>(statement)) return expressionMutatesName(expression->value, wanted);
    if (auto deferred = std::dynamic_pointer_cast<Defer>(statement)) return expressionMutatesName(deferred->action, wanted);
    if (auto ret = std::dynamic_pointer_cast<Return>(statement)) return expressionMutatesName(ret->value, wanted);
    if (std::dynamic_pointer_cast<Asm>(statement)) return true;
    if (auto gc = std::dynamic_pointer_cast<Gc>(statement)) return gc->body && statementMutatesName(gc->body, wanted);
    if (auto branch = std::dynamic_pointer_cast<If>(statement)) return expressionMutatesName(branch->condition, wanted) || statementMutatesName(branch->yes, wanted) || statementMutatesName(branch->no, wanted);
    if (auto loop = std::dynamic_pointer_cast<While>(statement)) return expressionMutatesName(loop->condition, wanted) || statementMutatesName(loop->body, wanted);
    if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
        if (expressionMutatesName(selection->value, wanted)) return true;
        for (const auto& arm : selection->cases) if (expressionMutatesName(arm.value, wanted) || statementMutatesName(arm.body, wanted)) return true;
        return statementMutatesName(selection->fallback, wanted);
    }
    if (auto block = std::dynamic_pointer_cast<Block>(statement)) {
        for (const auto& item : block->items) if (statementMutatesName(item, wanted)) return true;
    }
    return false;
}
}
std::string Codegen::name(const std::string& source) const {
    std::string result; result.reserve(source.size());
    for (char c : source) result += c == '.' ? '_' : c;
    return "xeUser_" + result;
}
std::string Codegen::quote(const std::string& value) const {
    std::string result = "\"";
    for (char c : value) { switch (c) { case '\\': result += "\\\\"; break; case '"': result += "\\\""; break; case '\n': result += "\\n"; break; case '\r': result += "\\r"; break; case '\t': result += "\\t"; break; default: result += c; } }
    return result + '"';
}
std::string Codegen::type(const std::string& source) const {
    if (source == "int") return "std::int64_t";
    if (source == "flt") return "double";
    if (source == "str" || source == "string") return "std::string";
    if (source == "void") return "void";
    if (!source.empty() && source.front() == '*') return type(source.substr(1)) + "*";
    if (!source.empty() && source.front() == '[') { auto end = source.find(']'); return "std::vector<" + type(source.substr(end + 1)) + ">"; }
    if (!source.empty() && (source.find('.') != std::string::npos || std::isalpha(static_cast<unsigned char>(source.front())))) return name(source);
    throw std::runtime_error("cannot generate C++ type for " + source);
}
std::string Codegen::defaultValue(const std::string& source) const {
    if (!source.empty() && source.front() == '[') {
        auto end = source.find(']'); auto count = source.substr(1, end - 1); auto element = source.substr(end + 1);
        return type(source) + "(" + count + ", " + defaultValue(element) + ")";
    }
    if (source == "str" || source == "string") return "std::string{}";
    if (!source.empty() && source.front() == '*') return "nullptr";
    if (source == "flt") return "double{}";
    if (source == "int") return "std::int64_t{}";
    return type(source) + "{}";
}
bool Codegen::isLvalue(const E& node) const {
    if (std::dynamic_pointer_cast<Name>(node) || std::dynamic_pointer_cast<Index>(node) || std::dynamic_pointer_cast<Member>(node)) return true;
    if (auto unary = std::dynamic_pointer_cast<Unary>(node)) return unary->op == "*" || unary->op == "#";
    return false;
}
std::string Codegen::expression(const E& node) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(node)) {
        if (literal->string) return quote(literal->value);
        if (literal->value.find_first_of(".eE") == std::string::npos) return std::to_string(std::stoll(literal->value));
        return literal->value;
    }
    if (auto value = std::dynamic_pointer_cast<Name>(node)) {
        if (value->value == "true") return "true";
        if (value->value == "false") return "false";
        return name(value->value);
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(node)) {
        if (unary->op == "@") return "xe::runtime::address(" + expression(unary->operand) + ")";
        if (unary->op == "#") return "xe::runtime::hardware(" + expression(unary->operand) + ")";
        if (unary->op == "-") if (auto literal = std::dynamic_pointer_cast<Literal>(unary->operand); literal && !literal->string && literal->value.find_first_of(".eE") == std::string::npos && std::stoull(literal->value) == static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1)
            return "std::numeric_limits<std::int64_t>::min()";
        if (unary->op == "-") return "xe::runtime::negate(" + expression(unary->operand) + ")";
        if (unary->op == "~") return "static_cast<std::int64_t>(~static_cast<std::uint64_t>(" + expression(unary->operand) + "))";
        return "(" + unary->op + expression(unary->operand) + ")";
    }
    if (auto binary = std::dynamic_pointer_cast<Binary>(node)) {
        if (binary->op == "&&" || binary->op == "||") return "(" + expression(binary->left) + " " + binary->op + " " + expression(binary->right) + ")";
        auto id = std::to_string(expressionScopeId++);
        auto left = "xeLeft" + id, right = "xeRight" + id;
        std::string operation;
        if (binary->op == "+") operation = "xe::runtime::add(" + left + ", " + right + ")";
        else if (binary->op == "-") operation = "xe::runtime::subtract(" + left + ", " + right + ")";
        else if (binary->op == "*") operation = "xe::runtime::multiply(" + left + ", " + right + ")";
        else if (binary->op == "/") operation = "xe::runtime::divide(" + left + ", " + right + ")";
        else if (binary->op == "%") operation = "xe::runtime::remainder(" + left + ", " + right + ")";
        else if (binary->op == "<<" || binary->op == ">>") operation = "(" + left + " " + binary->op + " " + right + ")";
        else if (binary->op == "&" || binary->op == "|" || binary->op == "^") operation = "(" + left + " " + binary->op + " " + right + ")";
        else operation = "(" + left + " " + (binary->op == "=" ? "==" : binary->op) + " " + right + ")";
        return "([&]() { auto xeLeft" + id + " = " + expression(binary->left) + "; auto xeRight" + id + " = " + expression(binary->right) + "; return " + operation + "; }())";
    }
    if (auto call = std::dynamic_pointer_cast<Call>(node)) {
        if (call->name == "alloc" || call->name == "new") return "xe::runtime::alloc(" + expression(call->args.at(0)) + ")";
        if (call->name == "alloc.aligned" || call->name == "aligned_alloc") return "xe::runtime::allocAligned(" + expression(call->args.at(0)) + ", " + expression(call->args.at(1)) + ")";
        if (call->name == "atomic.load") return "xe::runtime::atomicLoad(" + expression(call->args.at(0)) + ")";
        if (call->name == "atomic.store") return "xe::runtime::atomicStore(" + expression(call->args.at(0)) + ", " + expression(call->args.at(1)) + ")";
        if (call->name == "atomic.compare_exchange") return "xe::runtime::atomicCompareExchange(" + expression(call->args.at(0)) + ", " + expression(call->args.at(1)) + ", " + expression(call->args.at(2)) + ")";
        if (call->name == "flt.toint") return "xe::runtime::floatToInt(" + expression(call->args.at(0)) + ")";
        if (call->name == "runtime.pointer.is_null") return "xe::runtime::pointerIsNull(" + expression(call->args.at(0)) + ")";
        std::string target = call->name == "runtime.pointer.read" ? "xe::runtime::pointerRead" : call->name == "runtime.pointer.write" ? "xe::runtime::pointerWrite" :
                     call->name == "runtime.pointer.cast.string" ? "xe::runtime::pointerAsString" : call->name == "runtime.pointer.free" ? "xe::runtime::freeRaw" :
                     call->name == "runtime.mem.copy" ? "xe::runtime::sysMemCopy" : call->name == "runtime.mem.set" ? "xe::runtime::sysMemSet" : call->name == "runtime.mem.compare" ? "xe::runtime::sysMemCompare" :
                     call->name == "runtime.hw.read8" ? "xe::runtime::sysHardwareRead8" : call->name == "runtime.hw.write8" ? "xe::runtime::sysHardwareWrite8" :
                     call->name == "runtime.hw.read32" ? "xe::runtime::sysHardwareRead32" : call->name == "runtime.hw.write32" ? "xe::runtime::sysHardwareWrite32" :
                     call->name == "syscall.invoke3" ? "xe::runtime::syscallInvoke3" : call->name == "syscall.invoke6" ? "xe::runtime::syscallInvoke6" :
                     call->name == "flt.format" ? "xe::runtime::floatFormat" :
                             call->name == "arg.count" ? "xe::runtime::argCount" : call->name == "arg.get" ? "xe::runtime::arg" : call->name == "env.get" ? "xe::runtime::environmentValue" :
                             call->name == "file.read" ? "xe::runtime::readFile" : call->name == "file.write" ? "xe::runtime::writeFile" :
                             call->name == "process.run" ? "xe::runtime::runProcess" :
                             call->name == "process.spawn" ? "xe::runtime::spawnProcess" : call->name == "process.wait" ? "xe::runtime::waitProcess" :
                             call->name == "thread.start" ? "xe::runtime::threadStart" : call->name == "thread.join" ? "xe::runtime::threadJoin" :
                             call->name == "str.length" ? "xe::runtime::stringLength" : call->name == "str.char" ? "xe::runtime::stringChar" :
                             call->name == "str.builder" ? "xe::runtime::stringBuilder" : call->name == "str.append" ? "xe::runtime::stringAppend" :
                             call->name == "str.slice" ? "xe::runtime::stringSlice" : call->name == "str.find" ? "xe::runtime::stringFind" :
                             call->name == "str.number" ? "xe::runtime::stringNumber" : call->name == "str.fromchar" ? "xe::runtime::stringFromChar" :
                             (call->name == "alloc" || call->name == "free" || call->name == "delete" || call->name == "write" || call->name == "writeln" || call->name == "read" || call->name == "readln") ? "xe::runtime::" + (call->name == "delete" ? std::string("free") : call->name) :
                             call->name == "main" ? "main" : name(call->name);
        if (call->args.empty()) return target + "()";
        auto id = std::to_string(expressionScopeId++);
        std::string result = "([&]() { ";
        auto signature = parameterTypes.find(call->name);
        for (std::size_t i = 0; i < call->args.size(); ++i) {
            std::string argument = expression(call->args[i]);
            const auto* literal = std::dynamic_pointer_cast<Literal>(call->args[i]).get();
            if (signature != parameterTypes.end() && i < signature->second.size() && !signature->second[i].empty() && signature->second[i].front() == '*') {
                if (literal && !literal->string && literal->value == "0") argument = "nullptr";
                if (signature->second[i] == "*void") {
                    if (auto unary = std::dynamic_pointer_cast<Unary>(call->args[i]); unary && unary->op == "&") {
                        if (auto function = std::dynamic_pointer_cast<Name>(unary->operand); function && functionNames.contains(function->value))
                            argument = "reinterpret_cast<void*>(" + argument + ")";
                    }
                }
            }
            result += "auto&& xeArg" + id + "x" + std::to_string(i) + " = " + argument + "; ";
        }
        result += "return " + target + "(";
        for (std::size_t i = 0; i < call->args.size(); ++i) { if (i) result += ", "; result += "xeArg" + id + "x" + std::to_string(i); }
        return result + "); }())";
    }
    if (auto index = std::dynamic_pointer_cast<Index>(node)) {
        auto id = std::to_string(expressionScopeId++);
        auto base = "xeIndexBase" + id, position = "xeIndexPosition" + id;
        std::string lambda = "([&]()";
        if (isLvalue(index->base)) lambda += " -> decltype(auto)";
        lambda += " { auto&& " + base + " = " + expression(index->base) + "; auto " + position + " = " + expression(index->index) + "; return xe::runtime::at(" + base + ", " + position + "); }())";
        return lambda;
    }
    if (auto member = std::dynamic_pointer_cast<Member>(node)) return "(" + expression(member->base) + ")." + name(member->name).substr(7);
    if (auto slice = std::dynamic_pointer_cast<Slice>(node)) {
        auto id = std::to_string(expressionScopeId++);
        auto base = "xeSliceBase" + id;
        std::string result = "([&]() { auto&& " + base + " = " + expression(slice->base) + "; ";
        std::string first = "0", last = base + ".size()";
        if (slice->start) { auto start = "xeSliceStart" + id; result += "auto " + start + " = static_cast<std::size_t>(" + expression(slice->start) + "); "; first = start; }
        if (slice->finish) { auto finish = "xeSliceFinish" + id; result += "auto " + finish + " = static_cast<std::size_t>(" + expression(slice->finish) + "); "; last = finish; }
        return result + "return xe::runtime::slice(" + base + ", " + first + ", " + last + "); }())";
    }
    throw std::runtime_error("cannot generate C++ for expression");
}
void Codegen::block(const std::shared_ptr<Block>& body, std::string& out, int indent) {
    const std::string pad(static_cast<std::size_t>(indent), ' ');
    for (const auto& statement : body->items) {
        if (auto variable = std::dynamic_pointer_cast<Var>(statement)) {
            auto typeText = type(variable->type);
            const std::string qualifier = variable->immutable ? "const " : "";
            if (!variable->value && variable->type.front() == '[') {
                out += pad + qualifier + typeText + " " + name(variable->name) + " = " + defaultValue(variable->type) + ";\n";
            } else out += pad + qualifier + typeText + " " + name(variable->name) + (variable->value ? " = " + expression(variable->value) : "{}") + ";\n";
        } else if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) out += pad + expression(assignment->target) + " = " + expression(assignment->value) + ";\n";
        else if (auto expr = std::dynamic_pointer_cast<ExprStmt>(statement)) out += pad + expression(expr->value) + ";\n";
        else if (auto deferred = std::dynamic_pointer_cast<Defer>(statement)) {
            auto id = std::to_string(expressionScopeId++);
            out += pad + "auto xeDefer" + id + " = xe::runtime::Defer([&]() { " + expression(deferred->action) + "; });\n";
        }
        else if (auto ret = std::dynamic_pointer_cast<Return>(statement)) out += pad + "return" + (ret->value ? " " + expression(ret->value) : "") + ";\n";
        else if (auto gc = std::dynamic_pointer_cast<Gc>(statement)) { out += pad + "{\n" + pad + "  xe::runtime::GcScopeGuard xeGc;\n"; block(gc->body, out, indent + 2); out += pad + "}\n"; }
        else if (auto assembly = std::dynamic_pointer_cast<Asm>(statement)) {
            std::string delimiter = "XELANG"; while (assembly->text.find(")" + delimiter + '"') != std::string::npos) delimiter += "X";
            out += pad + "asm volatile (R\"" + delimiter + "(.intel_syntax noprefix\n" + assembly->text + "\n.att_syntax prefix)" + delimiter + "\" ::: \"memory\");\n";
        } else if (auto branch = std::dynamic_pointer_cast<If>(statement)) {
            out += pad + "if (" + expression(branch->condition) + ") {\n"; block(branch->yes, out, indent + 2); out += pad + "}";
            if (branch->no) { out += " else {\n"; block(branch->no, out, indent + 2); out += pad + "}"; } out += "\n";
        } else if (auto loop = std::dynamic_pointer_cast<While>(statement)) { out += pad + "while (" + expression(loop->condition) + ") {\n"; block(loop->body, out, indent + 2); out += pad + "}\n"; }
        else if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
            out += pad + "switch (" + expression(selection->value) + ") {\n";
            for (const auto& arm : selection->cases) { out += pad + "  case " + switchLabel(arm.value) + ": {\n"; block(arm.body, out, indent + 4); out += pad + "    break;\n" + pad + "  }\n"; }
            if (selection->fallback) { out += pad + "  default: {\n"; block(selection->fallback, out, indent + 4); out += pad + "    break;\n" + pad + "  }\n"; }
            out += pad + "}\n";
        }
        else if (auto nested = std::dynamic_pointer_cast<Block>(statement)) { out += pad + "{\n"; block(nested, out, indent + 2); out += pad + "}\n"; }
    }
}
std::string Codegen::generate(const Program& program) {
    expressionScopeId = 0;
    parameterTypes.clear();
    functionNames.clear();
    for (const auto& function : program.functions) {
        functionNames.insert(function.name);
        std::vector<std::string> types;
        for (const auto& parameter : function.params) types.push_back(parameter.type);
        parameterTypes.emplace(function.name, std::move(types));
    }
    std::string out = "#include <cstddef>\n#include <cstdint>\n#include <memory>\n#include <vector>\n#include \"runtime.hpp\"\n";
    for (const auto& record : program.records) out += "struct " + name(record.name) + ";\n";
    for (const auto& alias : program.aliases) out += "using " + name(alias.name) + " = " + type(alias.target) + ";\n";
    for (const auto& record : program.records) {
        out += "struct " + name(record.name) + " {\n";
        for (const auto& field : record.fields) out += "  " + type(field.type) + " " + name(field.name).substr(7) + " = " + defaultValue(field.type) + ";\n";
        out += "};\n";
    }
    std::vector<std::vector<bool>> readOnlyStringParameters(program.functions.size());
    for (std::size_t functionIndex = 0; functionIndex < program.functions.size(); ++functionIndex) {
        const auto& function = program.functions[functionIndex];
        auto& readOnly = readOnlyStringParameters[functionIndex];
        readOnly.resize(function.params.size(), false);
        for (std::size_t parameterIndex = 0; parameterIndex < function.params.size(); ++parameterIndex) {
            const auto& parameter = function.params[parameterIndex];
            if (parameter.type == "str" || parameter.type == "string") {
                readOnly[parameterIndex] = !statementMutatesName(function.body, parameter.name);
            }
        }
    }
    auto signature = [&](const Function& function, std::size_t functionIndex) {
        if (function.name == "main") return std::string("int main(int argc, char** argv)");
        std::string result = function.name == "main" && function.result == "int" ? "int" : type(function.result);
        result += " " + (function.name == "main" ? std::string("main") : name(function.name)) + "(";
        for (std::size_t i = 0; i < function.params.size(); ++i) {
            if (i) result += ", ";
            if (readOnlyStringParameters[functionIndex][i]) result += "const std::string& ";
            else result += type(function.params[i].type) + " ";
            result += name(function.params[i].name);
        }
        return result + ")";
    };
    for (std::size_t i = 0; i < program.functions.size(); ++i) out += signature(program.functions[i], i) + ";\n";
    for (std::size_t i = 0; i < program.functions.size(); ++i) {
        const auto& function = program.functions[i];
        out += signature(function, i) + " {\n";
        if (function.name == "main") out += "  xe::runtime::setArguments(argc, argv);\n";
        block(function.body, out, 2); out += "}\n";
    }
    return out;
}
}
