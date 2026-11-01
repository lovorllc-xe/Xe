#include "checker.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
namespace xe {
namespace {
bool numeric(const std::string& type) { return type == "int" || type == "flt"; }
bool stringLike(const std::string& type) { return type == "str" || type == "string"; }
bool allocationCall(const std::string& name) { return name == "alloc" || name == "alloc.aligned" || name == "new" || name == "aligned_alloc"; }
bool compatible(const std::string& to, const std::string& from) {
    return to == from || (to == "*void" && !from.empty() && from.front() == '*') ||
           (stringLike(to) && stringLike(from)) || (numeric(to) && numeric(from)) || (to == "void" && from == "void");
}
bool compatibleExpression(const std::string& to, const std::string& from, const E& expression) {
    if (compatible(to, from)) return true;
    if (!to.empty() && to.front() == '*' && from == "int") if (auto literal = std::dynamic_pointer_cast<Literal>(expression); literal && !literal->string && literal->value == "0") return true;
    auto call = std::dynamic_pointer_cast<Call>(expression);
    return call && allocationCall(call->name) && !to.empty() && to.front() == '*';
}
bool containsPointer(const std::string& type) {
    if (type.empty()) return false;
    if (type.front() == '*') return true;
    if (type.front() != '[') return false;
    auto end = type.find(']');
    return end != std::string::npos && containsPointer(type.substr(end + 1));
}
void collectNames(const E& expression, std::vector<std::string>& names) {
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) names.push_back(name->value);
    else if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) collectNames(unary->operand, names);
    else if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) { collectNames(binary->left, names); collectNames(binary->right, names); }
    else if (auto call = std::dynamic_pointer_cast<Call>(expression)) for (auto& arg : call->args) collectNames(arg, names);
}
std::optional<std::int64_t> integerConstant(const E& expression) {
    constexpr auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (auto literal = std::dynamic_pointer_cast<Literal>(expression); literal && !literal->string && literal->value.find_first_of(".eE") == std::string::npos) {
        auto magnitude = std::stoull(literal->value);
        if (magnitude <= maximum) return static_cast<std::int64_t>(magnitude);
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "-") {
        if (auto literal = std::dynamic_pointer_cast<Literal>(unary->operand); literal && !literal->string && literal->value.find_first_of(".eE") == std::string::npos) {
            auto magnitude = std::stoull(literal->value);
            if (magnitude == maximum + 1) return std::numeric_limits<std::int64_t>::min();
            if (magnitude <= maximum) return -static_cast<std::int64_t>(magnitude);
        }
    }
    return {};
}
bool alwaysReturns(const std::shared_ptr<Block>& block) {
    if (block->items.empty()) return false;
    const auto& last = block->items.back();
    if (std::dynamic_pointer_cast<Return>(last)) return true;
    if (auto branch = std::dynamic_pointer_cast<If>(last)) return branch->no && alwaysReturns(branch->yes) && alwaysReturns(branch->no);
    if (auto selection = std::dynamic_pointer_cast<Switch>(last)) { if (!selection->fallback || !alwaysReturns(selection->fallback)) return false; for (const auto& arm : selection->cases) if (!alwaysReturns(arm.body)) return false; return true; }
    if (auto nested = std::dynamic_pointer_cast<Block>(last)) return alwaysReturns(nested);
    return false;
}
/*
# MISSION & SPECIFICATION: STAGE 0 COMPILER & INTERPRETER FOR "MYLANG"

You are an expert compiler engineer. Your task is to implement a complete, fully functional Stage 0 compiler and interpreter for a new programming language ("XeLang", source file extension `.xe') written in modern C++20.

---

## 1. LANGUAGE DESIGN & SYNTAX SPECIFICATION

### A. General Formatting & Syntax Rules
1. **Strict Symbol Constraint:** Underlines (`_`) are ABSOLUTELY BANNED in language keywords, built-ins, standard library calls, syntax markers, variable names, and runtime calls. Dot notation (`.`) MUST be used for namespacing and scoping (e.g., `gc.begin`, `gc.end`).
2. **Line Termination & Continuations:**
   - Newlines naturally terminate statements.
   - Semicolons `;` are NOT required at line ends.
   - Semicolons `;` are strictly used as line-continuation markers to split a long statement across multiple lines.
3. **Block Hierarchy:**
   - Heavy structures (functions, modules, switch blocks) MUST use Pascal-style `begin` and `end`.
   - Small inline structures (loops, basic conditionals) MUST use curly braces `{` and `}`.
   - Arrays, indexing, and slice expressions use brackets `[` and `]`.
4. **Indentation:** The lexer and parser must support tabulated, clean visual structure without forcing strict Python-style whitespace errors.

### B. Types & Primitive Declarations
- `int` : 64-bit signed integer.
- `flt` : 64-bit double-precision float.
- `*T` : Raw pointer to type `T`.
- Type aliases, return types, and variables use Pascal-style `:` syntax (e.g., `var x: int = 10`, `fn calc(a: int): flt`).

### C. I/O & Built-ins
- Input/Output: `write`, `writeln`, `read`, `readln`.
- Special Symbols: `*`, `&`, `@`, `#`, `!` supported for low-level memory addressing, dereferencing, and direct hardware register access.

### D. Memory Management (Low-Level Default vs. Regional GC)
1. **Default Mode:** Bare-metal, zero-overhead manual allocation (`alloc`, `free`), pointers, and direct execution.
2. **Regional GC Mode:** High-level automatic memory management activated only inside specific blocks using `gc.begin` and `gc.end`. Allocations made inside a `gc.begin` ... `gc.end` region are tracked via a regional ARC/arena runtime and released automatically upon exiting the scope.

### E. Low-Level Features & Inline Assembly
- Direct inline assembly block support using `asm { ... }`. The compiler must extract raw assembly strings within `asm {}` and pass them straight to target output emission.

### G. Execution Modes
- **Interpreter Mode:** Execute source files directly via a Tree-Walk AST Evaluator / Bytecode VM (`mylang --run main.src`).
- **Compiler Mode:** Translate source code to C++ or LLVM IR and invoke host compilers to produce optimized native executables (`mylang --build main.src -o output`).

---

## 2. PROJECT FILE STRUCTURE

The C++20 project must be strictly structured into the following header/source pairs:

```text
mylang/
├── CMakeLists.txt          # Build system configuration
├── src/
│   ├── main.cpp            # Entry point, CLI flags (--run, --build)
*/
}
void Checker::checkType(const std::string& type) const {
    if (type == "int" || type == "flt" || type == "void" || type == "str" || type == "string") return;
    if (!type.empty() && type.front() == '*') { checkType(type.substr(1)); return; }
    if (!type.empty() && type.front() == '[') {
        auto end = type.find(']'); if (end == std::string::npos || end == 1) throw std::runtime_error("malformed array type: " + type);
        auto count = type.substr(1, end - 1); if (!std::all_of(count.begin(), count.end(), [](unsigned char c){return std::isdigit(c);}) || std::stoull(count) == 0) throw std::runtime_error("array length must be a positive integer");
        auto element = type.substr(end + 1);
        if (element == "void") throw std::runtime_error("arrays cannot have void elements");
        checkType(element); return;
    }
    if (records.contains(type)) return;
    throw std::runtime_error("unknown type: " + type);
}
const Checker::Symbol* Checker::findSymbol(const std::string& name) const {
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) { auto item = scope->find(name); if (item != scope->end()) return &item->second; }
    return nullptr;
}
const Checker::Symbol* Checker::storageSymbol(const E& expression) const {
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) return findSymbol(name->value);
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) return storageSymbol(index->base);
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) return storageSymbol(member->base);
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && (unary->op == "*" || unary->op == "&")) return storageSymbol(unary->operand);
    return nullptr;
}
int Checker::pointerRegion(const E& expression) const {
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) { auto symbol = findSymbol(name->value); return symbol && !symbol->type.empty() && symbol->type.front() == '*' ? symbol->pointerRegionDepth : -1; }
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        if (allocationCall(call->name)) return regionDepth;
        auto function = functions.find(call->name);
        if (function != functions.end() && !function->second->result.empty() && function->second->result.front() == '*') return regionDepth > 0 ? regionDepth : -1;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "&") {
        if (auto symbol = storageSymbol(unary->operand)) return symbol->regionDepth;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "*") return pointerRegion(unary->operand);
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) { if (auto symbol = storageSymbol(index->base); symbol && symbol->type.find('*') != std::string::npos) return symbol->regionDepth; }
    return -1;
}
bool Checker::functionMayRetainParameter(const std::string& name, std::size_t parameterIndex) const {
    auto found = functions.find(name);
    if (found == functions.end() || parameterIndex >= found->second->params.size()) return true;
    const std::string& parameter = found->second->params[parameterIndex].name;
    auto isParameter = [&](const E& expression) {
        auto variable = std::dynamic_pointer_cast<Name>(expression);
        if (variable) return variable->value == parameter;
        auto address = std::dynamic_pointer_cast<Unary>(expression);
        auto dereference = address && address->op == "&" ? std::dynamic_pointer_cast<Unary>(address->operand) : nullptr;
        auto inner = dereference && dereference->op == "*" ? std::dynamic_pointer_cast<Name>(dereference->operand) : nullptr;
        return inner && inner->value == parameter;
    };
    auto knownNonRetaining = [](const std::string& called) {
        return allocationCall(called) || called == "free" || called == "write" || called == "writeln" || called == "read" || called == "readln" ||
               called.rfind("str.", 0) == 0 || called.rfind("file.", 0) == 0 || called.rfind("arg.", 0) == 0 || called == "process.run" || called == "flt.format";
    };
    std::function<bool(const E&)> hasRetainingCall;
    hasRetainingCall = [&](const E& expression) -> bool {
        if (!expression) return false;
        if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
            for (const auto& argument : call->args) if (isParameter(argument) && !knownNonRetaining(call->name)) return true;
            for (const auto& argument : call->args) if (hasRetainingCall(argument)) return true;
        } else if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) return hasRetainingCall(unary->operand);
        else if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) return hasRetainingCall(binary->left) || hasRetainingCall(binary->right);
        else if (auto index = std::dynamic_pointer_cast<Index>(expression)) return hasRetainingCall(index->base) || hasRetainingCall(index->index);
        else if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) return hasRetainingCall(slice->base) || hasRetainingCall(slice->start) || hasRetainingCall(slice->finish);
        return false;
    };
    std::function<bool(const S&)> escapes;
    escapes = [&](const S& statement) -> bool {
        if (!statement) return false;
        if (std::dynamic_pointer_cast<Asm>(statement)) return true;
        if (auto variable = std::dynamic_pointer_cast<Var>(statement)) return isParameter(variable->value) || hasRetainingCall(variable->value);
        if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) return isParameter(assignment->value) || hasRetainingCall(assignment->value);
        if (auto result = std::dynamic_pointer_cast<Return>(statement)) return isParameter(result->value) || hasRetainingCall(result->value);
        if (auto expression = std::dynamic_pointer_cast<ExprStmt>(statement)) return hasRetainingCall(expression->value);
        if (auto block = std::dynamic_pointer_cast<Block>(statement)) {
            for (const auto& item : block->items) if (escapes(item)) return true;
        } else if (auto branch = std::dynamic_pointer_cast<If>(statement)) {
            return hasRetainingCall(branch->condition) || escapes(branch->yes) || escapes(branch->no);
        } else if (auto loop = std::dynamic_pointer_cast<While>(statement)) {
            return hasRetainingCall(loop->condition) || escapes(loop->body);
        } else if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
            if (hasRetainingCall(selection->value)) return true;
            for (const auto& arm : selection->cases) if (hasRetainingCall(arm.value) || escapes(arm.body)) return true;
            return escapes(selection->fallback);
        } else if (auto region = std::dynamic_pointer_cast<Gc>(statement)) return escapes(region->body);
        return false;
    };
    return escapes(found->second->body);
}
int Checker::pointerScope(const E& expression) const {
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        auto symbol = findSymbol(name->value);
        return symbol && !symbol->type.empty() && symbol->type.front() == '*' ? symbol->pointerScopeDepth : -1;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "&") {
        if (auto symbol = storageSymbol(unary->operand)) return symbol->scopeDepth;
    }
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        auto function = functions.find(call->name);
        if (function != functions.end() && !function->second->result.empty() && function->second->result.front() == '*') {
            std::unordered_set<std::string> active;
            if (functionReturnsOwnedPointer(call->name, active)) return -1;
            return scopes.empty() ? 0 : static_cast<int>(scopes.size()) - 1;
        }
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "*") return pointerScope(unary->operand);
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) {
        if (auto dereference = std::dynamic_pointer_cast<Unary>(member->base); dereference && dereference->op == "*" && heapOwnedPointer(dereference->operand)) return -1;
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) {
        if (auto member = std::dynamic_pointer_cast<Member>(index->base)) {
            if (auto dereference = std::dynamic_pointer_cast<Unary>(member->base); dereference && dereference->op == "*" && heapOwnedPointer(dereference->operand)) return -1;
        }
        if (auto symbol = storageSymbol(index->base); symbol && symbol->type.find('*') != std::string::npos) return symbol->scopeDepth;
    }
    return -1;
}
bool Checker::heapOwnedPointer(const E& expression) const {
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        if (allocationCall(call->name)) return true;
        auto function = functions.find(call->name);
        if (function == functions.end() || function->second->result.empty() || function->second->result.front() != '*') return false;
        std::unordered_set<std::string> active;
        return functionReturnsOwnedPointer(call->name, active);
    }
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        auto symbol = findSymbol(name->value);
        return symbol && symbol->heapOwned;
    }
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) {
        if (auto dereference = std::dynamic_pointer_cast<Unary>(member->base); dereference && dereference->op == "*")
            return heapOwnedPointer(dereference->operand);
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "*") {
        if (auto symbol = storageSymbol(expression)) return symbol->heapOwned;
    }
    return false;
}
bool Checker::functionReturnsOwnedPointer(const std::string& name, std::unordered_set<std::string>& active) const {
    auto found = functions.find(name);
    if (found == functions.end() || found->second->result.empty() || found->second->result.front() != '*' || !active.insert(name).second) return false;

    using State = std::unordered_map<std::string, std::optional<bool>>;
    using Environment = std::vector<State>;
    Environment environment(1);
    for (const auto& parameter : found->second->params) {
        if (!parameter.type.empty() && parameter.type.front() == '*') environment.front()[parameter.name] = false;
        else environment.front()[parameter.name] = std::nullopt;
    }
    bool sawReturn = false;
    bool allOwned = true;
    std::function<bool(const E&, const Environment&)> ownsExpression;
    ownsExpression = [&](const E& value, const Environment& state) {
        if (auto allocation = std::dynamic_pointer_cast<Call>(value)) {
            if (allocationCall(allocation->name)) return true;
            auto callee = functions.find(allocation->name);
            return callee != functions.end() && !callee->second->result.empty() && callee->second->result.front() == '*' && functionReturnsOwnedPointer(allocation->name, active);
        }
        if (auto variable = std::dynamic_pointer_cast<Name>(value)) {
            for (auto scope = state.rbegin(); scope != state.rend(); ++scope) {
                auto item = scope->find(variable->value);
                if (item != scope->end()) return item->second.value_or(false);
            }
        }
        return false;
    };
    auto merge = [](Environment& destination, const std::vector<Environment>& branches) {
        for (std::size_t scope = 0; scope < destination.size(); ++scope) {
            for (auto& [variable, state] : destination[scope]) {
                if (!state.has_value()) continue;
                bool ownedInEveryBranch = !branches.empty();
                for (const auto& branch : branches) {
                    if (scope >= branch.size()) { ownedInEveryBranch = false; break; }
                    auto item = branch[scope].find(variable);
                    if (item == branch[scope].end() || !item->second.value_or(false)) { ownedInEveryBranch = false; break; }
                }
                state = ownedInEveryBranch;
            }
        }
    };
    std::function<void(const std::shared_ptr<Block>&, Environment&, bool)> visitBlock;
    std::function<void(const S&, Environment&)> visitStatement;
    visitBlock = [&](const std::shared_ptr<Block>& block, Environment& state, bool scoped) {
        if (scoped) state.emplace_back();
        for (const auto& statement : block->items) visitStatement(statement, state);
        if (scoped) state.pop_back();
    };
    auto visitBranch = [&](const std::shared_ptr<Block>& block, Environment state) {
        visitBlock(block, state, true);
        return state;
    };
    visitStatement = [&](const S& statement, Environment& state) {
        if (auto variable = std::dynamic_pointer_cast<Var>(statement)) {
            if (!variable->type.empty() && variable->type.front() == '*') state.back()[variable->name] = variable->value && ownsExpression(variable->value, state);
            else state.back()[variable->name] = std::nullopt;
        } else if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) {
            if (auto variable = std::dynamic_pointer_cast<Name>(assignment->target)) {
                for (auto scope = state.rbegin(); scope != state.rend(); ++scope) {
                    auto item = scope->find(variable->value);
                    if (item != scope->end()) { if (item->second.has_value()) item->second = ownsExpression(assignment->value, state); break; }
                }
            }
        } else if (auto result = std::dynamic_pointer_cast<Return>(statement)) {
            sawReturn = true;
            if (!result->value || !ownsExpression(result->value, state)) allOwned = false;
        } else if (auto branch = std::dynamic_pointer_cast<If>(statement)) {
            std::vector<Environment> variants{visitBranch(branch->yes, state)};
            if (branch->no) variants.push_back(visitBranch(branch->no, state)); else variants.push_back(state);
            merge(state, variants);
        } else if (auto loop = std::dynamic_pointer_cast<While>(statement)) {
            auto body = visitBranch(loop->body, state); merge(state, {state, body});
        } else if (auto gc = std::dynamic_pointer_cast<Gc>(statement)) {
            auto body = visitBranch(gc->body, state); merge(state, {state, body});
        } else if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
            std::vector<Environment> variants;
            for (const auto& arm : selection->cases) variants.push_back(visitBranch(arm.body, state));
            if (selection->fallback) variants.push_back(visitBranch(selection->fallback, state)); else variants.push_back(state);
            merge(state, variants);
        } else if (auto nested = std::dynamic_pointer_cast<Block>(statement)) visitBlock(nested, state, true);
    };
    visitBlock(found->second->body, environment, false);
    active.erase(name);
    return sawReturn && allOwned;
}
bool Checker::addressOfRegionValue(const E& expression, int outerScopeDepth, int outerRegionDepth) const {
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "&") {
        if (auto symbol = storageSymbol(unary->operand); symbol && (symbol->scopeDepth > outerScopeDepth || symbol->regionDepth > outerRegionDepth)) return true;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) return addressOfRegionValue(unary->operand, outerScopeDepth, outerRegionDepth);
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) return addressOfRegionValue(binary->left, outerScopeDepth, outerRegionDepth) || addressOfRegionValue(binary->right, outerScopeDepth, outerRegionDepth);
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) return addressOfRegionValue(index->base, outerScopeDepth, outerRegionDepth) || addressOfRegionValue(index->index, outerScopeDepth, outerRegionDepth);
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) return addressOfRegionValue(slice->base, outerScopeDepth, outerRegionDepth) || (slice->start && addressOfRegionValue(slice->start, outerScopeDepth, outerRegionDepth)) || (slice->finish && addressOfRegionValue(slice->finish, outerScopeDepth, outerRegionDepth));
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) for (const auto& arg : call->args) if (addressOfRegionValue(arg, outerScopeDepth, outerRegionDepth)) return true;
    return false;
}
std::string Checker::checkExpression(const E& expression) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expression)) {
        if (literal->string) return "string";
        if (literal->value.find_first_of(".eE") == std::string::npos) {
            if (std::stoull(literal->value) > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) throw std::runtime_error("integer literal is out of range");
            return "int";
        }
        auto real = std::stod(literal->value);
        if (!std::isfinite(real)) throw std::runtime_error("floating-point literal is out of range");
        return "flt";
    }
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        if (name->value == "true" || name->value == "false") return "int";
        auto symbol = findSymbol(name->value); if (!symbol) throw std::runtime_error("unknown variable: " + name->value); return symbol->type;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) {
        if (unary->op == "-") if (auto literal = std::dynamic_pointer_cast<Literal>(unary->operand); literal && !literal->string && literal->value.find_first_of(".eE") == std::string::npos) {
            if (std::stoull(literal->value) > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1) throw std::runtime_error("integer literal is out of range");
            return "int";
        }
        auto operand = checkExpression(unary->operand);
        if (unary->op == "~") { if (operand != "int") throw std::runtime_error("bitwise complement requires an int operand"); return "int"; }
        if (unary->op == "!") { if (!numeric(operand) && (operand.empty() || operand.front() != '*')) throw std::runtime_error("logical not requires a numeric or pointer operand"); return "int"; }
        if (unary->op == "&") {
            bool assignable = static_cast<bool>(std::dynamic_pointer_cast<Name>(unary->operand)) || static_cast<bool>(std::dynamic_pointer_cast<Index>(unary->operand));
            if (auto target = std::dynamic_pointer_cast<Unary>(unary->operand); target && target->op == "*") assignable = true;
            if (auto target = std::dynamic_pointer_cast<Name>(unary->operand); target && (target->value == "true" || target->value == "false")) assignable = false;
            if (!assignable) throw std::runtime_error("address-of requires an assignable value");
            return "*" + operand;
        }
        if (unary->op == "*" && operand.size() && operand.front() == '*') {
            if (operand.substr(1) == "void") throw std::runtime_error("cannot dereference a void pointer without a concrete pointee type");
            return operand.substr(1);
        }
        if (unary->op == "@") { if (operand.empty() || operand.front() != '*') throw std::runtime_error("@ requires a pointer operand"); return "int"; }
        if (unary->op == "#") { if ((operand.empty() || operand.front() != '*') && operand != "int") throw std::runtime_error("# requires an address or pointer operand"); return "int"; }
        if (unary->op == "-" && numeric(operand)) return operand;
        throw std::runtime_error("invalid unary operator '" + unary->op + "' for " + operand);
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) {
        auto base = checkExpression(index->base); if (checkExpression(index->index) != "int") throw std::runtime_error("array index must have type int");
        if (base.empty() || base.front() != '[') throw std::runtime_error("indexing requires an array");
        auto end = base.find(']'); return base.substr(end + 1);
    }
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) {
        auto base = checkExpression(member->base);
        auto record = records.find(base);
        if (record == records.end()) throw std::runtime_error("field access requires a record value, got " + base);
        for (const auto& field : record->second->fields) if (field.name == member->name) return field.type;
        throw std::runtime_error("record " + base + " has no field " + member->name);
    }
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) {
        auto base = checkExpression(slice->base); if (base.empty() || base.front() != '[') throw std::runtime_error("slicing requires an array");
        if (slice->start && checkExpression(slice->start) != "int") throw std::runtime_error("slice start must have type int");
        if (slice->finish && checkExpression(slice->finish) != "int") throw std::runtime_error("slice end must have type int");
        return base;
    }
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) {
        auto left = checkExpression(binary->left), right = checkExpression(binary->right);
        if ((binary->op == "+" || binary->op == "-") && !left.empty() && left.front() == '*' && right == "int") return left;
        if (binary->op == "+" && left == "int" && !right.empty() && right.front() == '*') return right;
        if (binary->op == "&" || binary->op == "|" || binary->op == "^" || binary->op == "<<" || binary->op == ">>") {
            if (left != "int" || right != "int") throw std::runtime_error("bitwise operators require int operands");
            return "int";
        }
        if (binary->op == "&&" || binary->op == "||") { if (!numeric(left) || !numeric(right)) throw std::runtime_error("logical operators require numeric operands"); return "int"; }
        if (binary->op == "=" || binary->op == "!=") {
            bool nullLeft = !left.empty() && left.front() == '*' && compatibleExpression(left, right, binary->right);
            bool nullRight = !right.empty() && right.front() == '*' && compatibleExpression(right, left, binary->left);
            if ((numeric(left) && numeric(right)) || (stringLike(left) && stringLike(right)) ||
                (left == right && !left.empty() && left.front() == '*') || nullLeft || nullRight) return "int";
            throw std::runtime_error("equality operands have incompatible types");
        }
        if (binary->op == "<" || binary->op == ">" || binary->op == "<=" || binary->op == ">=") { if ((numeric(left) && numeric(right)) || (stringLike(left) && stringLike(right))) return "int"; throw std::runtime_error("ordered comparison requires compatible numeric or string operands"); }
        if (binary->op == "+" && stringLike(left) && stringLike(right)) return left;
        if (binary->op == "%" && (left != "int" || right != "int")) throw std::runtime_error("remainder requires integer operands");
        if (!numeric(left) || !numeric(right)) throw std::runtime_error("operator '" + binary->op + "' requires numeric operands");
        return left == "int" && right == "int" ? "int" : "flt";
    }
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        auto requireRawPointer = [&](std::size_t index) {
            if (index >= call->args.size()) throw std::runtime_error(call->name + " is missing an argument");
            auto type = checkExpression(call->args[index]);
            if (type.empty() || type.front() != '*') throw std::runtime_error(call->name + " expects a pointer argument");
        };
        auto requireIntValue = [&](std::size_t index) {
            if (index >= call->args.size() || checkExpression(call->args[index]) != "int") throw std::runtime_error(call->name + " expects integer arguments");
        };
        if (call->name == "runtime.pointer.read") {
            if (call->args.size() != 1) throw std::runtime_error(call->name + " expects one pointer-slot address");
            requireRawPointer(0);
            return "*void";
        }
        if (call->name == "runtime.pointer.cast.string") {
            if (call->args.size() != 1) throw std::runtime_error(call->name + " expects one pointer");
            requireRawPointer(0);
            return "*string";
        }
        if (call->name == "runtime.pointer.free") {
            if (call->args.size() != 1) throw std::runtime_error(call->name + " expects one pointer");
            requireRawPointer(0);
            return "void";
        }
        if (call->name == "runtime.pointer.write") {
            if (call->args.size() != 2) throw std::runtime_error(call->name + " expects a slot address and pointer value");
            requireRawPointer(0);
            auto valueType = checkExpression(call->args[1]);
            if (valueType.empty() || valueType.front() != '*') throw std::runtime_error(call->name + " expects a pointer value");
            return "void";
        }
        if (call->name == "runtime.mem.copy" || call->name == "runtime.mem.set" || call->name == "runtime.mem.compare") {
            if (call->args.size() != 3) throw std::runtime_error(call->name + " expects three arguments");
            requireRawPointer(0);
            if (call->name != "runtime.mem.set") requireRawPointer(1); else requireIntValue(1);
            requireIntValue(2);
            return call->name == "runtime.mem.compare" ? "int" : "void";
        }
        if (call->name == "runtime.hw.read8" || call->name == "runtime.hw.read32") {
            if (call->args.size() != 1) throw std::runtime_error(call->name + " expects one address");
            auto type = checkExpression(call->args[0]);
            if (type != "int" && (type.empty() || type.front() != '*')) throw std::runtime_error(call->name + " expects an address");
            return "int";
        }
        if (call->name == "runtime.hw.write8" || call->name == "runtime.hw.write32") {
            if (call->args.size() != 2) throw std::runtime_error(call->name + " expects an address and value");
            auto type = checkExpression(call->args[0]);
            if (type != "int" && (type.empty() || type.front() != '*')) throw std::runtime_error(call->name + " expects an address");
            requireIntValue(1);
            return "void";
        }
        if (call->name == "syscall.invoke3" || call->name == "syscall.invoke6") {
            const std::size_t count = call->name == "syscall.invoke3" ? 4 : 7;
            if (call->args.size() != count) throw std::runtime_error(call->name + " expects " + std::to_string(count) + " arguments");
            for (std::size_t index = 0; index < count; ++index) {
                auto argumentType = checkExpression(call->args[index]);
                if (index == 0 && argumentType != "int") throw std::runtime_error("syscall number must have type int");
                if (index > 0 && argumentType != "int" && !stringLike(argumentType) && (argumentType.empty() || argumentType.front() != '*')) throw std::runtime_error("syscall arguments must be ints, strings, or pointers");
            }
            return "int";
        }
        if (call->name == "write" || call->name == "writeln" || call->name == "read" || call->name == "readln" || call->name == "free" || call->name == "delete") {
            if ((call->name == "read" || call->name == "readln" || call->name == "free" || call->name == "delete") && call->args.size() != 1) throw std::runtime_error(call->name + " expects one argument");
            for (auto& argument : call->args) {
                auto argumentType = checkExpression(argument);
                if ((call->name == "write" || call->name == "writeln") && argumentType == "void") throw std::runtime_error("write cannot print a void expression");
                if ((call->name == "write" || call->name == "writeln") && !numeric(argumentType) && !stringLike(argumentType) && (argumentType.empty() || argumentType.front() != '*')) throw std::runtime_error("write supports primitive values and pointers");
                if ((call->name == "free" || call->name == "delete") && (argumentType.empty() || argumentType.front() != '*')) throw std::runtime_error(call->name + " requires a pointer");
                if (call->name == "free" || call->name == "delete") {
                    auto pointerName = std::dynamic_pointer_cast<Name>(argument);
                    bool recordAllocationField = false;
                    if (auto member = std::dynamic_pointer_cast<Member>(argument); member) {
                        if (auto dereference = std::dynamic_pointer_cast<Unary>(member->base); dereference && dereference->op == "*")
                            recordAllocationField = heapOwnedPointer(dereference->operand);
                    }
                    if ((!pointerName && !recordAllocationField) || (!heapOwnedPointer(argument) && !recordAllocationField)) throw std::runtime_error("free requires a pointer returned by alloc");
                    if (pointerName) {
                        for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
                            auto item = scope->find(pointerName->value);
                            if (item != scope->end()) {
                                item->second.heapOwned = false;
                                item->second.pointerRegionDepth = -1;
                                item->second.pointerScopeDepth = -1;
                                break;
                            }
                        }
                    }
                }
                if ((call->name == "read" || call->name == "readln") && !std::dynamic_pointer_cast<Name>(argument)) throw std::runtime_error(call->name + " requires a variable name");
                if (auto inputName = std::dynamic_pointer_cast<Name>(argument); inputName && (inputName->value == "true" || inputName->value == "false") && (call->name == "read" || call->name == "readln")) throw std::runtime_error(call->name + " requires a variable name");
                if (call->name == "read" && !numeric(argumentType)) throw std::runtime_error("read requires a numeric variable");
                if (call->name == "readln" && argumentType != "str" && argumentType != "string") throw std::runtime_error("readln requires a string variable");
            }
            return "void";
        }
        if (call->name == "alloc" || call->name == "new") { if (call->args.size() != 1) throw std::runtime_error(call->name + " expects one size argument"); if (checkExpression(call->args.front()) != "int") throw std::runtime_error("allocation size must have type int"); return "*void"; }
        if (call->name == "alloc.aligned" || call->name == "aligned_alloc") {
            if (call->args.size() != 2) throw std::runtime_error(call->name + " expects size and alignment arguments");
            if (checkExpression(call->args[0]) != "int" || checkExpression(call->args[1]) != "int") throw std::runtime_error("aligned allocation arguments must have type int");
            return "*void";
        }
        auto requireArity = [&](std::size_t count) {
            if (call->args.size() != count) throw std::runtime_error(call->name + " expects " + std::to_string(count) + " argument(s)");
        };
        auto requireString = [&](std::size_t index) {
            if (!stringLike(checkExpression(call->args[index]))) throw std::runtime_error(call->name + " expects string arguments");
        };
        auto requireInt = [&](std::size_t index) {
            if (checkExpression(call->args[index]) != "int") throw std::runtime_error(call->name + " expects integer arguments");
        };
        if (call->name == "flt.format") {
            requireArity(2);
            auto valueType = checkExpression(call->args[0]);
            if (!numeric(valueType)) throw std::runtime_error("flt.format expects a numeric value");
            requireInt(1);
            return "string";
        }
        if (call->name == "arg.count") { requireArity(0); return "int"; }
        if (call->name == "arg.get") { requireArity(1); requireInt(0); return "string"; }
        if (call->name == "file.read") { requireArity(1); requireString(0); return "string"; }
        if (call->name == "file.write") { requireArity(2); requireString(0); requireString(1); return "void"; }
        if (call->name == "process.run") { requireArity(1); requireString(0); return "int"; }
        if (call->name == "str.length") { requireArity(1); requireString(0); return "int"; }
        if (call->name == "str.builder") {
            if (call->args.size() > 1) throw std::runtime_error("str.builder expects zero or one string argument");
            if (!call->args.empty()) requireString(0);
            return "string";
        }
        if (call->name == "str.append") { requireArity(2); requireString(0); requireString(1); return "string"; }
        if (call->name == "str.find") { requireArity(2); requireString(0); requireString(1); return "int"; }
        if (call->name == "str.number") { requireArity(1); requireInt(0); return "string"; }
        if (call->name == "str.fromchar") { requireArity(1); requireInt(0); return "string"; }
        if (call->name == "str.char") { requireArity(2); requireString(0); requireInt(1); return "int"; }
        if (call->name == "str.slice") { requireArity(3); requireString(0); requireInt(1); requireInt(2); return "string"; }
        auto function = functions.find(call->name); if (function == functions.end()) throw std::runtime_error("unknown function: " + call->name);
        if (call->args.size() != function->second->params.size()) throw std::runtime_error("wrong number of arguments to " + call->name);
        for (std::size_t i = 0; i < call->args.size(); ++i) {
            auto argumentType = checkExpression(call->args[i]); const auto& parameterType = function->second->params[i].type;
            if (!compatibleExpression(parameterType, argumentType, call->args[i])) throw std::runtime_error("argument type mismatch in call to " + call->name);
            if (regionDepth > 0 && !argumentType.empty() && argumentType.front() == '*' && pointerRegion(call->args[i]) >= regionDepth && functionMayRetainParameter(call->name, i))
                throw std::runtime_error("GC-owned pointer data cannot be passed to a function that may retain it");
            if (regionDepth > 0 && !argumentType.empty() && argumentType.front() == '[' && containsPointer(argumentType))
                throw std::runtime_error("arrays containing GC-owned pointers cannot be passed to functions");
        }
        return function->second->result;
    }
    throw std::runtime_error("invalid expression");
}
void Checker::checkBlock(const std::shared_ptr<Block>& block, bool newScope) {
    if (newScope) scopes.emplace_back();
    for (const auto& statement : block->items) checkStatement(statement);
    if (newScope) scopes.pop_back();
}
void Checker::checkStatement(const S& statement) {
    if (auto variable = std::dynamic_pointer_cast<Var>(statement)) {
        checkType(variable->type); if (variable->type == "void") throw std::runtime_error("variables cannot have type void");
        if (variable->name == "true" || variable->name == "false") throw std::runtime_error("true and false are reserved boolean literals");
        if (scopes.back().contains(variable->name)) throw std::runtime_error("duplicate variable in scope: " + variable->name);
        if (variable->value) { auto valueType = checkExpression(variable->value); if (!compatibleExpression(variable->type, valueType, variable->value)) throw std::runtime_error("cannot initialize " + variable->name + " of type " + variable->type + " with " + valueType); }
        int pointerDepth = -1;
        if (!variable->type.empty() && variable->type.front() == '*') pointerDepth = pointerRegion(variable->value);
        Symbol symbol{variable->type, regionDepth, pointerDepth};
        symbol.scopeDepth = static_cast<int>(scopes.size()) - 1;
        if (!variable->type.empty() && variable->type.front() == '*') {
            symbol.pointerScopeDepth = pointerScope(variable->value);
            symbol.heapOwned = heapOwnedPointer(variable->value);
        }
        symbol.immutable = variable->immutable;
        scopes.back().emplace(variable->name, std::move(symbol)); return;
    }
    if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) {
        if (auto target = std::dynamic_pointer_cast<Name>(assignment->target); target && (target->value == "true" || target->value == "false")) throw std::runtime_error("boolean literals are not assignable");
        bool assignable = static_cast<bool>(std::dynamic_pointer_cast<Name>(assignment->target)) || static_cast<bool>(std::dynamic_pointer_cast<Index>(assignment->target)) || static_cast<bool>(std::dynamic_pointer_cast<Member>(assignment->target));
        if (auto unary = std::dynamic_pointer_cast<Unary>(assignment->target); unary && (unary->op == "*" || unary->op == "#")) assignable = true;
        if (!assignable) throw std::runtime_error("left side of assignment is not assignable");
        auto targetType = checkExpression(assignment->target), valueType = checkExpression(assignment->value);
        if (!compatibleExpression(targetType, valueType, assignment->value)) throw std::runtime_error("assignment type mismatch: " + targetType + " and " + valueType);
        if (!std::dynamic_pointer_cast<Unary>(assignment->target)) {
            if (auto destinationName = std::dynamic_pointer_cast<Name>(assignment->target)) {
                if (auto symbol = findSymbol(destinationName->value); symbol && symbol->immutable) throw std::runtime_error("cannot assign to const value: " + destinationName->value);
            } else if (std::dynamic_pointer_cast<Member>(assignment->target) || std::dynamic_pointer_cast<Index>(assignment->target)) {
                E base = assignment->target;
                while (auto member = std::dynamic_pointer_cast<Member>(base)) base = member->base;
                while (auto index = std::dynamic_pointer_cast<Index>(base)) base = index->base;
                if (!std::dynamic_pointer_cast<Unary>(base)) if (auto baseName = std::dynamic_pointer_cast<Name>(base)) {
                    if (auto symbol = findSymbol(baseName->value); symbol && symbol->immutable) throw std::runtime_error("cannot modify a field or element of const value: " + baseName->value);
                }
            }
        }
        auto destination = storageSymbol(assignment->target);
        if (destination) {
            if (pointerRegion(assignment->value) > destination->regionDepth ||
                pointerScope(assignment->value) > destination->scopeDepth ||
                addressOfRegionValue(assignment->value, destination->scopeDepth, destination->regionDepth))
                throw std::runtime_error("pointer or address from a shorter-lived scope cannot escape into an outer variable");
            std::vector<std::string> names; collectNames(assignment->value, names);
            for (const auto& name : names) {
                auto source = findSymbol(name);
                if (source && (source->regionDepth > destination->regionDepth || source->scopeDepth > destination->scopeDepth) &&
                    !source->type.empty() && source->type.front() == '[' && containsPointer(source->type)) {
                    throw std::runtime_error("array containing pointers cannot escape from a shorter-lived scope");
                }
            }
        }
        if (auto target = std::dynamic_pointer_cast<Name>(assignment->target); target && !targetType.empty() && targetType.front() == '*') {
            for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
                auto item = scope->find(target->value);
                if (item != scope->end()) {
                    item->second.pointerRegionDepth = pointerRegion(assignment->value);
                    item->second.pointerScopeDepth = pointerScope(assignment->value);
                    item->second.heapOwned = heapOwnedPointer(assignment->value);
                    break;
                }
            }
        }
        return;
    }
    if (auto expression = std::dynamic_pointer_cast<ExprStmt>(statement)) { checkExpression(expression->value); return; }
    if (auto deferred = std::dynamic_pointer_cast<Defer>(statement)) {
        if (checkExpression(deferred->action) != "void" || !std::dynamic_pointer_cast<Call>(deferred->action))
            throw std::runtime_error("defer requires a procedure call");
        return;
    }
    if (auto result = std::dynamic_pointer_cast<Return>(statement)) {
        std::string valueType = result->value ? checkExpression(result->value) : "void";
        if (!compatibleExpression(currentResult, valueType, result->value)) throw std::runtime_error("return type mismatch: function returns " + currentResult + " but expression has type " + valueType);
        if (result->value && !valueType.empty() && valueType.front() == '[' && containsPointer(valueType)) throw std::runtime_error("arrays containing pointers cannot be returned");
        if (result->value && !valueType.empty() && valueType.front() == '*' &&
            ((regionDepth > 0 && pointerRegion(result->value) >= regionDepth) || pointerScope(result->value) >= 0 || addressOfRegionValue(result->value, -1, -1)))
            throw std::runtime_error("pointer to local or GC-owned memory cannot be returned");
        return;
    }
    if (auto gc = std::dynamic_pointer_cast<Gc>(statement)) { ++regionDepth; checkBlock(gc->body); --regionDepth; return; }
    if (auto branch = std::dynamic_pointer_cast<If>(statement)) { auto condition = checkExpression(branch->condition); if (!numeric(condition) && (condition.empty() || condition.front() != '*')) throw std::runtime_error("if condition must be numeric or a pointer"); checkBlock(branch->yes); if (branch->no) checkBlock(branch->no); return; }
    if (auto loop = std::dynamic_pointer_cast<While>(statement)) { auto condition = checkExpression(loop->condition); if (!numeric(condition) && (condition.empty() || condition.front() != '*')) throw std::runtime_error("while condition must be numeric or a pointer"); checkBlock(loop->body); return; }
    if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
        if (checkExpression(selection->value) != "int") throw std::runtime_error("switch expression must have type int");
        std::unordered_set<std::int64_t> labels;
        for (const auto& arm : selection->cases) {
            if (checkExpression(arm.value) != "int") throw std::runtime_error("switch case labels must have type int");
            auto label = integerConstant(arm.value); if (!label) throw std::runtime_error("switch case labels must be integer constants");
            if (!labels.insert(*label).second) throw std::runtime_error("duplicate switch case label");
            checkBlock(arm.body);
        }
        if (selection->fallback) checkBlock(selection->fallback);
        return;
    }
    if (auto nested = std::dynamic_pointer_cast<Block>(statement)) { checkBlock(nested); return; }
    if (std::dynamic_pointer_cast<Asm>(statement)) return;
    throw std::runtime_error("unsupported statement in type checker");
}
void Checker::check(const Program& program) {
    functions.clear(); records.clear(); scopes.clear(); regionDepth = 0;
    for (const auto& record : program.records) if (!records.emplace(record.name, &record).second) throw std::runtime_error("duplicate record type: " + record.name);
    for (const auto& record : program.records) {
        std::unordered_set<std::string> fields;
        for (const auto& field : record.fields) {
            if (!fields.insert(field.name).second) throw std::runtime_error("duplicate field in record " + record.name + ": " + field.name);
            checkType(field.type);
        }
    }
    std::unordered_set<std::string> aliasNames;
    for (const auto& alias : program.aliases) { if (!aliasNames.insert(alias.name).second) throw std::runtime_error("duplicate type alias: " + alias.name); checkType(alias.target); }
    for (const auto& function : program.functions) { if (!functions.emplace(function.name, &function).second) throw std::runtime_error("duplicate function: " + function.name); checkType(function.result); for (const auto& param : function.params) checkType(param.type); }
    if (!functions.contains("main")) throw std::runtime_error("program must define fn main");
    if (functions.at("main")->result != "int" || !functions.at("main")->params.empty()) throw std::runtime_error("main must have the signature fn main(): int");
    for (const auto& function : program.functions) {
        currentResult = function.result;
        scopes.emplace_back(); for (const auto& param : function.params) {
            if (param.type == "void") throw std::runtime_error("parameters cannot have type void");
            if (param.name == "true" || param.name == "false") throw std::runtime_error("true and false are reserved boolean literals");
            if (scopes.back().contains(param.name)) throw std::runtime_error("duplicate parameter: " + param.name);
            Symbol symbol{param.type, 0}; symbol.scopeDepth = 0;
            symbol.heapOwned = !param.type.empty() && param.type.front() == '*';
            scopes.back().emplace(param.name, std::move(symbol));
        }
        if (function.result != "void" && !alwaysReturns(function.body)) throw std::runtime_error("function may finish without returning a value: " + function.name);
        checkBlock(function.body, false); scopes.pop_back();
    }
}
}
