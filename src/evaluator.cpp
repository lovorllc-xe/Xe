#include "evaluator.hpp"
#include "runtime.hpp"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cctype>
#include <chrono>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <mutex>
#include <new>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#if defined(__unix__) || defined(__APPLE__)
#include <dlfcn.h>
#endif

#ifndef XELANG_CXX_COMPILER
#define XELANG_CXX_COMPILER "c++"
#endif

namespace xe {
struct AllocationArena {
    struct Chunk { std::unique_ptr<std::byte[]> data; std::size_t capacity; };
    std::vector<Chunk> chunks;
    std::size_t used = 0;
    std::mutex mutex;

    std::byte* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        std::lock_guard lock(mutex);
        if (alignment < alignof(void*) || (alignment & (alignment - 1)) != 0)
            throw std::invalid_argument("alignment must be a power of two at least alignof(void*)");
        constexpr std::size_t defaultChunkSize = 64 * 1024;
        auto fits = [&](const Chunk& chunk) {
            auto base = reinterpret_cast<std::uintptr_t>(chunk.data.get());
            auto address = base + used;
            auto aligned = (address + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
            auto offset = static_cast<std::size_t>(aligned - base);
            return offset <= chunk.capacity && size <= chunk.capacity - offset;
        };
        if (chunks.empty() || !fits(chunks.back())) {
            if (size > std::numeric_limits<std::size_t>::max() - alignment) throw std::bad_alloc();
            auto capacity = std::max(defaultChunkSize, size + alignment);
            chunks.push_back({std::make_unique<std::byte[]>(capacity), capacity});
            used = 0;
        }
        auto base = reinterpret_cast<std::uintptr_t>(chunks.back().data.get());
        auto aligned = (base + used + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
        used = static_cast<std::size_t>(aligned - base) + size;
        return reinterpret_cast<std::byte*>(aligned);
    }
};
Allocation::~Allocation() {
    if (alignedOwned) ::operator delete(alignedOwned, std::align_val_t(alignment));
}
namespace {
struct ReturnSignal { Value value; };
std::string shellQuote(const std::string& value) {
    std::string quoted = "'";
    for (char character : value) { if (character == '\'') quoted += "'\\''"; else quoted += character; }
    return quoted + "'";
}
std::int64_t syscallWord(const Value& value) {
    if (auto integer = std::get_if<std::int64_t>(&value)) return *integer;
    if (auto string = std::get_if<std::string>(&value)) return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(string->c_str()));
    if (auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value); pointer && *pointer && (*pointer)->live)
        return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>((*pointer)->data));
    if (std::holds_alternative<std::monostate>(value)) return 0;
    throw std::runtime_error("syscall arguments must be integers or raw pointers");
}
struct CompiledAssembly {
#if defined(__unix__) || defined(__APPLE__)
    void* handle = nullptr;
    void (*entry)() = nullptr;
    ~CompiledAssembly() { if (handle) dlclose(handle); }
#endif
};
void executeInlineAssembly(const std::string& assembly) {
#if defined(__unix__) || defined(__APPLE__)
    static std::mutex cacheMutex;
    static std::unordered_map<std::string, std::shared_ptr<CompiledAssembly>> cache;
    static std::atomic<std::uint64_t> nextAssemblyId{0};
    std::shared_ptr<CompiledAssembly> compiled;
    {
        std::lock_guard lock(cacheMutex);
        auto found = cache.find(assembly);
        if (found != cache.end()) compiled = found->second;
        else {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto id = nextAssemblyId.fetch_add(1, std::memory_order_relaxed);
            auto base = std::filesystem::temp_directory_path() / ("xelang-asm-" + std::to_string(stamp) + "-" + std::to_string(id));
            auto sourcePath = base; sourcePath += ".cpp";
#if defined(__APPLE__)
            auto libraryPath = base; libraryPath += ".dylib";
#else
            auto libraryPath = base; libraryPath += ".so";
#endif
            std::string delimiter = "XELANG";
            while (assembly.find(")" + delimiter + '"') != std::string::npos) delimiter += "X";
            {
                std::ofstream source(sourcePath, std::ios::binary);
                if (!source) throw std::runtime_error("cannot create temporary source for inline assembly");
                source << "extern \"C\" void xelang_asm_entry() { asm volatile (R\"" << delimiter << "(.intel_syntax noprefix\n" << assembly << "\n.att_syntax prefix)" << delimiter << "\" ::: \"memory\"); }\n";
            }
#if defined(__APPLE__)
            const std::string sharedFlags = " -dynamiclib -fPIC";
#else
            const std::string sharedFlags = " -shared -fPIC";
#endif
            const std::string command = shellQuote(XELANG_CXX_COMPILER) + " -std=c++20" + sharedFlags + " " + shellQuote(sourcePath.string()) + " -o " + shellQuote(libraryPath.string());
            const int status = std::system(command.c_str());
            std::error_code ignored;
            std::filesystem::remove(sourcePath, ignored);
            if (status != 0) { std::filesystem::remove(libraryPath, ignored); throw std::runtime_error("failed to compile host inline assembly for --run"); }
            auto loaded = std::make_shared<CompiledAssembly>();
            loaded->handle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_LOCAL);
            if (!loaded->handle) {
                const char* loaderError = dlerror();
                std::string message = loaderError ? loaderError : "unknown dynamic loader error";
                std::filesystem::remove(libraryPath, ignored);
                throw std::runtime_error("cannot load inline assembly: " + message);
            }
            dlerror();
            loaded->entry = reinterpret_cast<void (*)()>(dlsym(loaded->handle, "xelang_asm_entry"));
            if (const char* error = dlerror(); error || !loaded->entry) {
                loaded.reset(); std::filesystem::remove(libraryPath, ignored);
                throw std::runtime_error("cannot find inline assembly entry point: " + std::string(error ? error : "null symbol"));
            }
            std::filesystem::remove(libraryPath, ignored);
            compiled = std::move(loaded);
            cache.emplace(assembly, compiled);
        }
    }
    compiled->entry();
#else
    (void)assembly;
    throw std::runtime_error("inline assembly JIT is supported only on POSIX hosts");
#endif
}
double number(const Value& value) {
    if (auto integer = std::get_if<std::int64_t>(&value)) return static_cast<double>(*integer);
    if (auto real = std::get_if<double>(&value)) return *real;
    throw std::runtime_error("numeric value required");
}
std::size_t arrayPosition(const Value& value) {
    if (auto integer = std::get_if<std::int64_t>(&value)) {
        if (*integer < 0 || static_cast<std::uint64_t>(*integer) > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("array index is out of range");
        return static_cast<std::size_t>(*integer);
    }
    if (auto real = std::get_if<double>(&value)) {
        if (!std::isfinite(*real) || *real < 0 || std::trunc(*real) != *real || static_cast<long double>(*real) > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("array index must be a non-negative integer");
        return static_cast<std::size_t>(*real);
    }
    throw std::runtime_error("array index must have type int");
}
bool sameReference(const RuntimeReference& left, const RuntimeReference& right) {
    if (left.arrayElement || right.arrayElement) return left.arrayElement && right.arrayElement && left.array.get() == right.array.get() && left.index == right.index;
    return left.scopeIndex == right.scopeIndex && left.name == right.name;
}
bool samePointer(const Value& left, const Value& right) {
    auto isNull = [](const Value& value) {
        if (std::holds_alternative<std::monostate>(value)) return true;
        if (auto integer = std::get_if<std::int64_t>(&value)) return *integer == 0;
        return false;
    };
    bool leftNull = isNull(left);
    bool rightNull = isNull(right);
    if (leftNull || rightNull) return leftNull && rightNull;
    if (auto a = std::get_if<std::shared_ptr<Allocation>>(&left)) {
        if (auto b = std::get_if<std::shared_ptr<Allocation>>(&right)) return *a && *b && (*a)->data == (*b)->data;
        return false;
    }
    if (auto a = std::get_if<std::shared_ptr<RuntimeReference>>(&left)) {
        if (auto b = std::get_if<std::shared_ptr<RuntimeReference>>(&right)) return *a && *b && sameReference(**a, **b);
    }
    return false;
}
Value cloneArrayValue(const Value& value) {
    if (auto array = std::get_if<std::shared_ptr<RuntimeArray>>(&value); array && *array) {
        auto copy = std::make_shared<RuntimeArray>(); copy->values.reserve((*array)->values.size());
        for (const auto& element : (*array)->values) copy->values.push_back(cloneArrayValue(element));
        return copy;
    }
    return value;
}
bool truth(const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) return false;
    if (auto integer = std::get_if<std::int64_t>(&value)) return *integer != 0;
    if (auto real = std::get_if<double>(&value)) return *real != 0.0;
    if (auto string = std::get_if<std::string>(&value)) return !string->empty();
    return true;
}
Value coerceValue(Value value, const std::string& type) {
    if (type == "int") {
        if (auto integer = std::get_if<std::int64_t>(&value)) return *integer;
        long double numericValue = number(value);
        if (!std::isfinite(numericValue) || numericValue >= static_cast<long double>(std::numeric_limits<std::int64_t>::max()) + 1.0L || numericValue < static_cast<long double>(std::numeric_limits<std::int64_t>::min())) throw std::runtime_error("integer value is out of range");
        return static_cast<std::int64_t>(numericValue);
    }
    if (type == "flt") return number(value);
    if (!type.empty() && type.front() == '[') return cloneArrayValue(value);
    if (!type.empty() && type.front() == '*') {
        if (auto integer = std::get_if<std::int64_t>(&value); integer && *integer == 0) return std::monostate{};
        if (auto allocation = std::get_if<std::shared_ptr<Allocation>>(&value); allocation && *allocation) {
            auto pointee = type.substr(1);
            std::size_t required = 0;
            if (pointee == "int") required = sizeof(std::int64_t); 
            else if (pointee == "flt") required = sizeof(double);
            else if (pointee == "str" || pointee == "string") required = sizeof(std::string);
            else if (!pointee.empty() && pointee.front() == '*') required = sizeof(void*);
            else if (!pointee.empty() && pointee.front() == '[') required = sizeof(std::vector<std::int64_t>);
            if ((*allocation)->size < required) throw std::runtime_error("allocation is too small for the requested pointer type");
        }
        return value;
    }
    if (type == "str" || type == "string") { if (!std::holds_alternative<std::string>(value)) throw std::runtime_error("string value required"); return value; }
    return value;
}
void printValue(std::ostream& out, const Value& value) {
    if (std::holds_alternative<std::monostate>(value)) out << '0';
    if (auto integer = std::get_if<std::int64_t>(&value)) out << *integer;
    else if (auto real = std::get_if<double>(&value)) { auto precision = out.precision(); out << std::setprecision(std::numeric_limits<double>::max_digits10) << *real; out.precision(precision); }
    else if (auto string = std::get_if<std::string>(&value)) out << *string;
    else if (std::holds_alternative<std::shared_ptr<RuntimeArray>>(value)) out << "[array]";
    else if (std::holds_alternative<std::shared_ptr<RuntimeRecord>>(value)) out << "[record]";
    else if (auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value)) out << (*pointer && (*pointer)->live ? static_cast<void*>((*pointer)->data) : nullptr);
    else out << "[pointer]";
}
}

Evaluator::Evaluator(std::istream& in, std::ostream& out, std::vector<std::string> argv) : input(in), output(out), arguments(std::move(argv)) {}
Value Evaluator::defaultValue(const std::string& type) {
    if (!type.empty() && type.front() == '[') {
        auto end = type.find(']'); auto count = static_cast<std::size_t>(std::stoull(type.substr(1, end - 1)));
        auto array = std::make_shared<RuntimeArray>(); array->values.reserve(count);
        for (std::size_t i = 0; i < count; ++i) array->values.push_back(defaultValue(type.substr(end + 1)));
        return array;
    }
    if (type == "str" || type == "string") return std::string{};
    if (type == "flt") return 0.0;
    if (!type.empty() && type.front() == '*') return std::monostate{};
    if (auto definition = recordTypes.find(type); definition != recordTypes.end()) {
        auto value = std::make_shared<RuntimeRecord>();
        for (const auto& field : definition->second->fields) value->fields.emplace(field.name, defaultValue(field.type));
        return value;
    }
    return std::int64_t{0};
}
Value Evaluator::lookup(const std::string& name) const {
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        auto item = scope->find(name); if (item != scope->end()) return item->second;
    }
    throw std::runtime_error("unknown variable: " + name);
}
std::string Evaluator::expressionType(const E& expression) const {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expression)) {
        if (literal->string) return "string";
        return literal->value.find_first_of(".eE") == std::string::npos ? "int" : "flt";
    }
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        if (name->value == "true" || name->value == "false") return "int";
        for (auto scope = types.rbegin(); scope != types.rend(); ++scope) {
            auto item = scope->find(name->value);
            if (item != scope->end()) return item->second;
        }
        return {};
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) {
        auto operand = expressionType(unary->operand);
        if (unary->op == "&") return "*" + operand;
        if (unary->op == "*") return !operand.empty() && operand.front() == '*' ? operand.substr(1) : std::string{};
        if (unary->op == "@" || unary->op == "#" || unary->op == "!") return "int";
        return operand;
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) {
        auto base = expressionType(index->base);
        if (!base.empty() && base.front() == '[') { auto end = base.find(']'); if (end != std::string::npos) return base.substr(end + 1); }
        return {};
    }
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) {
        auto definition = recordTypes.find(expressionType(member->base));
        if (definition != recordTypes.end()) for (const auto& field : definition->second->fields) if (field.name == member->name) return field.type;
        return {};
    }
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) return expressionType(slice->base);
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) {
        auto left = expressionType(binary->left), right = expressionType(binary->right);
        if ((binary->op == "+" || binary->op == "-") && !left.empty() && left.front() == '*' && right == "int") return left;
        if (binary->op == "+" && left == "int" && !right.empty() && right.front() == '*') return right;
        if (binary->op == "&&" || binary->op == "||" || binary->op == "=" || binary->op == "!=" || binary->op == "<" || binary->op == ">" || binary->op == "<=" || binary->op == ">=") return "int";
        if (binary->op == "+" && (left == "str" || left == "string") && (right == "str" || right == "string")) return "string";
        return left == "int" && right == "int" ? "int" : "flt";
    }
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) {
        if (call->name == "runtime.pointer.cast.string") return "*string";
        if (call->name == "alloc" || call->name == "alloc.aligned" || call->name == "new" || call->name == "aligned_alloc") return "*void";
        if (call->name == "flt.format") return "string";
        if (call->name == "arg.count" || call->name == "str.char" || call->name == "str.length" || call->name == "str.find" || call->name == "process.run") return "int";
        if (call->name == "arg.get" || call->name == "file.read" || call->name == "str.slice" || call->name == "str.number" || call->name == "str.fromchar" || call->name == "str.builder" || call->name == "str.append") return "string";
        if (call->name == "write" || call->name == "writeln" || call->name == "read" || call->name == "readln" || call->name == "free" || call->name == "file.write") return "void";
        auto function = functions.find(call->name);
        return function == functions.end() ? std::string{} : function->second->result;
    }
    return {};
}
std::int64_t Evaluator::hardwareValue(const Value& value) const {
    if (auto integer = std::get_if<std::int64_t>(&value)) return *integer;
    if (auto real = std::get_if<double>(&value)) return std::bit_cast<std::int64_t>(*real);
    if (auto allocation = std::get_if<std::shared_ptr<Allocation>>(&value)) return *allocation && (*allocation)->data ? static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>((*allocation)->data)) : 0;
    if (std::holds_alternative<std::monostate>(value)) return 0;
    if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&value); reference && *reference) {
        const Value* slot = nullptr;
        if ((*reference)->arrayElement) slot = &(*reference)->array->values.at((*reference)->index);
        else if ((*reference)->scopeIndex < scopes.size()) {
            auto item = scopes[(*reference)->scopeIndex].find((*reference)->name);
            if (item != scopes[(*reference)->scopeIndex].end()) slot = &item->second;
        }
        if (slot) return hardwareValue(*slot);
    }
    throw std::runtime_error("interpreter hardware access supports 64-bit primitive or pointer values");
}
void Evaluator::storeHardwareValue(Value& target, std::int64_t value) {
    if (std::holds_alternative<std::int64_t>(target)) { target = value; return; }
    if (std::holds_alternative<double>(target)) { target = std::bit_cast<double>(value); return; }
    if (std::holds_alternative<std::monostate>(target)) {
        if (!value) return;
        throw std::runtime_error("interpreter cannot construct a pointer from an arbitrary hardware address");
    }
    if (std::holds_alternative<std::shared_ptr<Allocation>>(target)) {
        if (!value) { target = std::monostate{}; return; }
        throw std::runtime_error("interpreter cannot construct a pointer from an arbitrary hardware address");
    }
    if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&target); reference && *reference) {
        Value* slot = nullptr;
        if ((*reference)->arrayElement) slot = &(*reference)->array->values.at((*reference)->index);
        else if ((*reference)->scopeIndex < scopes.size()) {
            auto item = scopes[(*reference)->scopeIndex].find((*reference)->name);
            if (item != scopes[(*reference)->scopeIndex].end()) slot = &item->second;
        }
        if (slot) { storeHardwareValue(*slot, value); return; }
    }
    throw std::runtime_error("interpreter hardware access supports 64-bit primitive or pointer values");
}
Value Evaluator::evaluate(const E& expression) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expression)) {
        if (literal->string) return literal->value;
        if (literal->value.find_first_of(".eE") != std::string::npos) return std::stod(literal->value);
        return static_cast<std::int64_t>(std::stoll(literal->value));
    }
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        if (name->value == "true") return std::int64_t{1};
        if (name->value == "false") return std::int64_t{0};
        return lookup(name->value);
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression)) {
        if (unary->op == "&") return addressOf(unary->operand);
        if (unary->op == "-") if (auto literal = std::dynamic_pointer_cast<Literal>(unary->operand); literal && !literal->string && literal->value.find_first_of(".eE") == std::string::npos) {
            auto magnitude = std::stoull(literal->value);
            auto maximum = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
            if (magnitude > maximum + 1) throw std::runtime_error("integer literal is out of range");
            if (magnitude == maximum + 1) return std::numeric_limits<std::int64_t>::min();
            return -static_cast<std::int64_t>(magnitude);
        }
        Value value = evaluate(unary->operand);
        if (unary->op == "-") {
            if (auto integer = std::get_if<std::int64_t>(&value)) {
                if (*integer == std::numeric_limits<std::int64_t>::min()) throw std::runtime_error("integer overflow");
                return -*integer;
            }
            return -number(value);
        }
        if (unary->op == "!") return std::int64_t{!truth(value)};
        if (unary->op == "@") {
            if (std::holds_alternative<std::monostate>(value)) return std::int64_t{0};
            if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&value); reference && *reference) {
                Value* slot = nullptr;
                if ((*reference)->arrayElement) slot = &(*reference)->array->values.at((*reference)->index);
                else if ((*reference)->scopeIndex < scopes.size()) { auto item = scopes[(*reference)->scopeIndex].find((*reference)->name); if (item != scopes[(*reference)->scopeIndex].end()) slot = &item->second; }
                if (slot) return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(slot));
            }
            auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
            if (!pointer || !*pointer || !(*pointer)->live || !(*pointer)->data) throw std::runtime_error("@ requires a live allocated pointer");
            return static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>((*pointer)->data));
        }
        if (unary->op == "#") {
            if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&value); reference && *reference) {
                if ((*reference)->arrayElement) return hardwareValue((*reference)->array->values.at((*reference)->index));
                if ((*reference)->scopeIndex < scopes.size()) {
                    auto item = scopes[(*reference)->scopeIndex].find((*reference)->name);
                    if (item != scopes[(*reference)->scopeIndex].end()) return hardwareValue(item->second);
                }
            }
            auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
            if (auto address = std::get_if<std::int64_t>(&value)) { std::lock_guard lock(*hardwareMutex); return (*hardwareRegisters)[*address]; }
            if (!pointer || !*pointer || !(*pointer)->live || (*pointer)->size < sizeof(std::int64_t)) throw std::runtime_error("interpreter hardware access requires a live pointer to allocated memory");
            std::int64_t result; std::memcpy(&result, (*pointer)->data, sizeof(result)); return result;
        }
        if (unary->op == "~") {
            auto integer = std::get_if<std::int64_t>(&value);
            if (!integer) throw std::runtime_error("bitwise complement requires an int operand");
            return static_cast<std::int64_t>(~static_cast<std::uint64_t>(*integer));
        }
        if (unary->op == "*") {
            if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&value); reference && *reference) {
                if ((*reference)->arrayElement) return (*reference)->array->values.at((*reference)->index);
                if ((*reference)->scopeIndex < scopes.size()) { auto item = scopes[(*reference)->scopeIndex].find((*reference)->name); if (item != scopes[(*reference)->scopeIndex].end()) return item->second; }
                throw std::runtime_error("pointer refers to an expired variable");
            }
            auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
            if (!pointer || !*pointer || !(*pointer)->live || !(*pointer)->data || ((*pointer)->backing && !(*pointer)->backing->live)) throw std::runtime_error("invalid pointer dereference");
            auto root = (*pointer)->backing ? (*pointer)->backing : *pointer;
            if (auto cell = root->cells.find((*pointer)->byteOffset); cell != root->cells.end()) return cell->second;
            if ((*pointer)->cellSet) return (*pointer)->cell;
            auto targetType = expressionType(expression);
            if (targetType == "flt") {
                if ((*pointer)->size < sizeof(double)) throw std::runtime_error("allocation is too small to dereference a flt");
                double result; std::memcpy(&result, (*pointer)->data, sizeof(result)); return result;
            }
            if (targetType == "string" || targetType == "str") return std::string{};
            if (!targetType.empty() && targetType.front() == '[') return std::make_shared<RuntimeArray>();
            if (recordTypes.contains(targetType)) { (*pointer)->cell = defaultValue(targetType); (*pointer)->cellSet = true; root->cells[(*pointer)->byteOffset] = (*pointer)->cell; return (*pointer)->cell; }
            if (!targetType.empty() && targetType.front() == '*') {
                if ((*pointer)->size < sizeof(std::uintptr_t)) throw std::runtime_error("allocation is too small to dereference a pointer");
                std::uintptr_t raw; std::memcpy(&raw, (*pointer)->data, sizeof(raw));
                if (!raw) return std::monostate{};
                throw std::runtime_error("interpreter cannot reconstruct a raw pointer written outside XeLang");
            }
            if (targetType != "int") throw std::runtime_error("interpreter cannot dereference this allocated type");
            if ((*pointer)->size < sizeof(std::int64_t)) throw std::runtime_error("allocation is too small to dereference an int");
            std::int64_t result; std::memcpy(&result, (*pointer)->data, sizeof(result)); return result;
        }
    }
    if (auto binary = std::dynamic_pointer_cast<Binary>(expression)) {
        Value left = evaluate(binary->left);
        if (binary->op == "&&" && !truth(left)) return std::int64_t{0};
        if (binary->op == "||" && truth(left)) return std::int64_t{1};
        Value right = evaluate(binary->right);
        if (binary->op == "&&") return std::int64_t{truth(right)};
        if (binary->op == "||") return std::int64_t{truth(right)};
        if (binary->op == "+" || binary->op == "-") {
            auto offsetPointer = [&](const std::shared_ptr<Allocation>& pointer, std::int64_t amount, const E& pointerExpr) -> Value {
                if (!pointer || !pointer->live || !pointer->data || (pointer->backing && !pointer->backing->live)) throw std::runtime_error("pointer arithmetic requires a live allocation");
                const auto pointerType = expressionType(pointerExpr);
                const auto element = pointerType.size() > 0 && pointerType.front() == '*' ? pointerType.substr(1) : std::string{};
                const std::size_t stride = element == "void" ? 1 : sizeof(std::int64_t);
                if (binary->op == "-") {
                    if (amount == std::numeric_limits<std::int64_t>::min()) throw std::runtime_error("pointer offset is out of range");
                    amount = -amount;
                }
                const auto root = pointer->backing ? pointer->backing : pointer;
                const auto current = static_cast<std::int64_t>(pointer->byteOffset);
                if (amount > 0 && static_cast<std::uint64_t>(amount) > (root->size - pointer->byteOffset) / stride) throw std::runtime_error("pointer arithmetic exceeds allocation bounds");
                if (amount < 0 && static_cast<std::uint64_t>(-(amount + 1)) + 1 > pointer->byteOffset / stride) throw std::runtime_error("pointer arithmetic exceeds allocation bounds");
                const auto next = static_cast<std::size_t>(current + amount * static_cast<std::int64_t>(stride));
                auto view = std::make_shared<Allocation>(); view->backing = root; view->byteOffset = next;
                view->data = root->data + next; view->size = root->size - next; view->alignment = pointer->alignment; return view;
            };
            if (auto pointer = std::get_if<std::shared_ptr<Allocation>>(&left); pointer && std::get_if<std::int64_t>(&right))
                return offsetPointer(*pointer, std::get<std::int64_t>(right), binary->left);
            if (binary->op == "+") if (auto pointer = std::get_if<std::shared_ptr<Allocation>>(&right); pointer && std::get_if<std::int64_t>(&left))
                return offsetPointer(*pointer, std::get<std::int64_t>(left), binary->right);
        }
        if (binary->op == "+") {
            if (auto a = std::get_if<std::string>(&left)) { auto b = std::get_if<std::string>(&right); if (!b) throw std::runtime_error("cannot concatenate string and number"); return *a + *b; }
        }
        if (auto leftString = std::get_if<std::string>(&left)) if (auto rightString = std::get_if<std::string>(&right)) {
            if (binary->op == "=") return std::int64_t{*leftString == *rightString};
            if (binary->op == "!=") return std::int64_t{*leftString != *rightString};
            if (binary->op == "<") return std::int64_t{*leftString < *rightString};
            if (binary->op == ">") return std::int64_t{*leftString > *rightString};
            if (binary->op == "<=") return std::int64_t{*leftString <= *rightString};
            if (binary->op == ">=") return std::int64_t{*leftString >= *rightString};
        }
        if (auto a = std::get_if<std::int64_t>(&left)) if (auto b = std::get_if<std::int64_t>(&right)) {
            if (binary->op == "&") return static_cast<std::int64_t>(static_cast<std::uint64_t>(*a) & static_cast<std::uint64_t>(*b));
            if (binary->op == "|") return static_cast<std::int64_t>(static_cast<std::uint64_t>(*a) | static_cast<std::uint64_t>(*b));
            if (binary->op == "^") return static_cast<std::int64_t>(static_cast<std::uint64_t>(*a) ^ static_cast<std::uint64_t>(*b));
            if (binary->op == "<<" || binary->op == ">>") {
                if (*b < 0 || *b >= 64) throw std::runtime_error("shift count must be between 0 and 63");
                if (binary->op == "<<") return static_cast<std::int64_t>(static_cast<std::uint64_t>(*a) << *b);
                return *a >> *b;
            }
            if (binary->op == "=") return std::int64_t{*a == *b};
            if (binary->op == "!=") return std::int64_t{*a != *b};
            if (binary->op == "<") return std::int64_t{*a < *b};
            if (binary->op == ">") return std::int64_t{*a > *b};
            if (binary->op == "<=") return std::int64_t{*a <= *b};
            if (binary->op == ">=") return std::int64_t{*a >= *b};
            if (binary->op == "+") { if ((*b > 0 && *a > std::numeric_limits<std::int64_t>::max() - *b) || (*b < 0 && *a < std::numeric_limits<std::int64_t>::min() - *b)) throw std::runtime_error("integer overflow"); return *a + *b; }
            if (binary->op == "-") { if ((*b < 0 && *a > std::numeric_limits<std::int64_t>::max() + *b) || (*b > 0 && *a < std::numeric_limits<std::int64_t>::min() + *b)) throw std::runtime_error("integer overflow"); return *a - *b; }
            if (binary->op == "*") { long double product = static_cast<long double>(*a) * *b; if (product > std::numeric_limits<std::int64_t>::max() || product < std::numeric_limits<std::int64_t>::min()) throw std::runtime_error("integer overflow"); return static_cast<std::int64_t>(product); }
            if (binary->op == "/") { if (!*b) throw std::runtime_error("division by zero"); if (*a == std::numeric_limits<std::int64_t>::min() && *b == -1) throw std::runtime_error("integer overflow"); return *a / *b; }
            if (binary->op == "%") { if (!*b) throw std::runtime_error("division by zero"); if (*a == std::numeric_limits<std::int64_t>::min() && *b == -1) return std::int64_t{0}; return *a % *b; }
        }
        if (binary->op == "=" || binary->op == "!=") {
            if (std::holds_alternative<std::monostate>(left) || std::holds_alternative<std::monostate>(right) ||
                std::holds_alternative<std::shared_ptr<Allocation>>(left) || std::holds_alternative<std::shared_ptr<Allocation>>(right) ||
                std::holds_alternative<std::shared_ptr<RuntimeReference>>(left) || std::holds_alternative<std::shared_ptr<RuntimeReference>>(right))
                return std::int64_t{samePointer(left, right) == (binary->op == "=")};
        }
        double a = number(left), b = number(right);
        if (binary->op == "+") return a + b;
        if (binary->op == "-") return a - b;
        if (binary->op == "*") return a * b;
        if (binary->op == "/") return a / b;
        if (binary->op == "%") { if (b == 0) throw std::runtime_error("division by zero"); return static_cast<std::int64_t>(a) % static_cast<std::int64_t>(b); }
        if (binary->op == "=") return std::int64_t{a == b};
        if (binary->op == "!=") return std::int64_t{a != b};
        if (binary->op == "<") return std::int64_t{a < b};
        if (binary->op == ">") return std::int64_t{a > b};
        if (binary->op == "<=") return std::int64_t{a <= b};
        if (binary->op == ">=") return std::int64_t{a >= b};
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) {
        Value base = evaluate(index->base);
        auto actual = std::get_if<std::shared_ptr<RuntimeArray>>(&base);
        if (!actual || !*actual) throw std::runtime_error("indexing requires an array");
        auto i = arrayPosition(evaluate(index->index));
        if (i >= (*actual)->values.size()) throw std::runtime_error("array index out of bounds");
        return (*actual)->values[i];
    }
    if (auto member = std::dynamic_pointer_cast<Member>(expression)) {
        Value base = evaluate(member->base); auto record = std::get_if<std::shared_ptr<RuntimeRecord>>(&base);
        if (!record || !*record) throw std::runtime_error("field access requires a record value");
        auto field = (*record)->fields.find(member->name);
        if (field == (*record)->fields.end()) throw std::runtime_error("record has no field " + member->name);
        return field->second;
    }
    if (auto slice = std::dynamic_pointer_cast<Slice>(expression)) {
        Value base = evaluate(slice->base); auto array = std::get_if<std::shared_ptr<RuntimeArray>>(&base);
        if (!array || !*array) throw std::runtime_error("slicing requires an array");
        auto first = slice->start ? arrayPosition(evaluate(slice->start)) : 0;
        auto last = slice->finish ? arrayPosition(evaluate(slice->finish)) : (*array)->values.size();
        if (first > last || last > (*array)->values.size()) throw std::runtime_error("array slice out of bounds");
        auto result = std::make_shared<RuntimeArray>(); result->values.reserve(last - first);
        for (auto i = first; i < last; ++i) result->values.push_back(cloneArrayValue((*array)->values[i]));
        return result;
    }
    if (auto call = std::dynamic_pointer_cast<Call>(expression)) return invoke(call->name, call->args);
    throw std::runtime_error("invalid expression");
}
Value Evaluator::addressOf(const E& expression) {
    if (auto unary = std::dynamic_pointer_cast<Unary>(expression); unary && unary->op == "*") return evaluate(unary->operand);
    if (auto name = std::dynamic_pointer_cast<Name>(expression)) {
        for (std::size_t i = scopes.size(); i > 0; --i) if (scopes[i - 1].contains(name->value)) {
            auto reference = std::make_shared<RuntimeReference>(); reference->scopeIndex = i - 1; reference->name = name->value;
            auto type = types[i - 1].find(name->value); if (type != types[i - 1].end()) reference->type = type->second;
            return reference;
        }
        throw std::runtime_error("address of unknown variable: " + name->value);
    }
    if (auto index = std::dynamic_pointer_cast<Index>(expression)) {
        Value base = evaluate(index->base); auto array = std::get_if<std::shared_ptr<RuntimeArray>>(&base);
        if (!array || !*array) throw std::runtime_error("address of an indexed value requires an array");
        auto i = arrayPosition(evaluate(index->index)); if (i >= (*array)->values.size()) throw std::runtime_error("array index out of bounds");
        auto reference = std::make_shared<RuntimeReference>(); reference->array = *array; reference->index = i; reference->arrayElement = true;
        reference->type = expressionType(expression);
        return reference;
    }
    throw std::runtime_error("address-of requires an assignable value");
}
void Evaluator::assign(const E& target, Value value) {
    if (auto name = std::dynamic_pointer_cast<Name>(target)) {
        for (std::size_t n = scopes.size(); n > 0; --n) { auto item = scopes[n - 1].find(name->value); if (item != scopes[n - 1].end()) { auto type = types[n - 1].find(name->value); if (type != types[n - 1].end()) value = coerceValue(std::move(value), type->second); item->second = std::move(value); return; } }
        throw std::runtime_error("assignment to unknown variable: " + name->value);
    }
    if (auto index = std::dynamic_pointer_cast<Index>(target)) {
        Value base = evaluate(index->base); auto array = std::get_if<std::shared_ptr<RuntimeArray>>(&base);
        if (!array || !*array) throw std::runtime_error("indexed assignment requires an array");
        auto i = arrayPosition(evaluate(index->index));
        if (i >= (*array)->values.size()) throw std::runtime_error("array index out of bounds");
        value = coerceValue(std::move(value), expressionType(index));
        (*array)->values[i] = std::move(value); return;
    }
    if (auto member = std::dynamic_pointer_cast<Member>(target)) {
        Value base = evaluate(member->base); auto record = std::get_if<std::shared_ptr<RuntimeRecord>>(&base);
        if (!record || !*record) throw std::runtime_error("field assignment requires a record value");
        auto field = (*record)->fields.find(member->name);
        if (field == (*record)->fields.end()) throw std::runtime_error("record has no field " + member->name);
        value = coerceValue(std::move(value), expressionType(member)); field->second = std::move(value); return;
    }
    if (auto unary = std::dynamic_pointer_cast<Unary>(target); unary && (unary->op == "*" || unary->op == "#")) {
        auto pointerValue = evaluate(unary->operand); auto pointer = std::get_if<std::shared_ptr<Allocation>>(&pointerValue);
        if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&pointerValue); reference && *reference) {
            if (unary->op == "#") {
                Value* slot = nullptr;
                if ((*reference)->arrayElement) slot = &(*reference)->array->values.at((*reference)->index);
                else if ((*reference)->scopeIndex < scopes.size()) {
                    auto item = scopes[(*reference)->scopeIndex].find((*reference)->name);
                    if (item != scopes[(*reference)->scopeIndex].end()) slot = &item->second;
                }
                if (!slot) throw std::runtime_error("pointer refers to an expired variable");
                storeHardwareValue(*slot, static_cast<std::int64_t>(number(value)));
                return;
            }
            if (!(*reference)->type.empty()) value = coerceValue(std::move(value), (*reference)->type);
            if ((*reference)->arrayElement) { (*reference)->array->values.at((*reference)->index) = std::move(value); return; }
            if ((*reference)->scopeIndex < scopes.size()) { auto item = scopes[(*reference)->scopeIndex].find((*reference)->name); if (item != scopes[(*reference)->scopeIndex].end()) { item->second = std::move(value); return; } }
            throw std::runtime_error("pointer refers to an expired variable");
        }
        if (unary->op == "#") if (auto address = std::get_if<std::int64_t>(&pointerValue)) {
            { std::lock_guard lock(*hardwareMutex); (*hardwareRegisters)[*address] = static_cast<std::int64_t>(number(value)); }
            return;
        }
        if (!pointer || !*pointer || !(*pointer)->live) throw std::runtime_error("invalid pointer store");
        auto targetType = unary->op == "#" ? std::string("int") : expressionType(target);
        Value stored = coerceValue(std::move(value), targetType);
        if (targetType == "flt") {
            if ((*pointer)->size < sizeof(double)) throw std::runtime_error("allocation is too small to store a flt");
            auto real = number(stored); std::memcpy((*pointer)->data, &real, sizeof(real));
        } else if (!targetType.empty() && targetType.front() == '*') {
            if ((*pointer)->size < sizeof(std::uintptr_t)) throw std::runtime_error("allocation is too small to store a pointer");
            std::uintptr_t address = 0;
            if (auto allocation = std::get_if<std::shared_ptr<Allocation>>(&stored); allocation && *allocation) address = reinterpret_cast<std::uintptr_t>((*allocation)->data);
            if (auto reference = std::get_if<std::shared_ptr<RuntimeReference>>(&stored); reference && *reference) {
                if ((*reference)->arrayElement) address = reinterpret_cast<std::uintptr_t>(&(*reference)->array->values.at((*reference)->index));
                else if ((*reference)->scopeIndex < scopes.size()) {
                    auto item = scopes[(*reference)->scopeIndex].find((*reference)->name);
                    if (item != scopes[(*reference)->scopeIndex].end()) address = reinterpret_cast<std::uintptr_t>(&item->second);
                }
            }
            std::memcpy((*pointer)->data, &address, sizeof(address));
        } else if (targetType == "int") {
            if ((*pointer)->size < sizeof(std::int64_t)) throw std::runtime_error("allocation is too small to store an int");
            auto integer = std::get<std::int64_t>(stored); std::memcpy((*pointer)->data, &integer, sizeof(integer));
        } else if (targetType == "string" || targetType == "str" || (!targetType.empty() && targetType.front() == '[')) {
            (*pointer)->cell = std::move(stored); (*pointer)->cellSet = true;
            auto root = (*pointer)->backing ? (*pointer)->backing : *pointer; root->cells[(*pointer)->byteOffset] = (*pointer)->cell; return;
        } else throw std::runtime_error("cannot store this type through an allocated pointer");
        if (unary->op == "#") { (*pointer)->cellSet = false; return; }
        (*pointer)->cell = std::move(stored); (*pointer)->cellSet = true;
        auto root = (*pointer)->backing ? (*pointer)->backing : *pointer; root->cells[(*pointer)->byteOffset] = (*pointer)->cell; return;
    }
    throw std::runtime_error("invalid assignment target");
}
Value Evaluator::invoke(const std::string& name, const std::vector<E>& arguments) {
    if (name == "runtime.pointer.cast.string") {
        if (arguments.size() != 1) throw std::runtime_error(name + " expects one pointer");
        return evaluate(arguments[0]);
    }
    if (name == "runtime.pointer.free") {
        if (arguments.size() != 1) throw std::runtime_error(name + " expects one pointer");
        Value value = evaluate(arguments[0]);
        auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
        if (!pointer || !*pointer || !(*pointer)->live || !(*pointer)->data) throw std::runtime_error(name + " requires a live allocated pointer");
        auto root = (*pointer)->backing ? (*pointer)->backing : *pointer;
        auto found = std::find(manualAllocations.begin(), manualAllocations.end(), root);
        if (found == manualAllocations.end()) throw std::runtime_error(name + " cannot release region-owned memory");
        root->live = false;
        root->owned.reset();
        if (root->alignedOwned) {
            ::operator delete(root->alignedOwned, std::align_val_t(root->alignment));
            root->alignedOwned = nullptr;
        }
        root->data = nullptr;
        root->size = 0;
        root->cells.clear();
        manualAllocations.erase(found);
        return std::monostate{};
    }
    if (name == "runtime.pointer.read" || name == "runtime.pointer.write") {
        bool write = name == "runtime.pointer.write";
        if (arguments.size() != (write ? 2u : 1u)) throw std::runtime_error(name + " received the wrong number of arguments");
        Value addressValue = evaluate(arguments[0]);
        auto pointer = std::get_if<std::shared_ptr<Allocation>>(&addressValue);
        if (!pointer || !*pointer || !(*pointer)->live || !(*pointer)->data) throw std::runtime_error(name + " requires a live allocated pointer");
        if ((*pointer)->size < sizeof(void*)) throw std::runtime_error(name + " exceeds allocation bounds");
        auto root = (*pointer)->backing ? (*pointer)->backing : *pointer;
        if (write) {
            Value value = evaluate(arguments[1]);
            if (!std::holds_alternative<std::monostate>(value) && !std::holds_alternative<std::shared_ptr<Allocation>>(value) && !std::holds_alternative<std::shared_ptr<RuntimeReference>>(value)) throw std::runtime_error(name + " expects a pointer value");
            root->cells[(*pointer)->byteOffset] = std::move(value);
            return std::monostate{};
        }
        auto cell = root->cells.find((*pointer)->byteOffset);
        return cell == root->cells.end() ? Value{std::monostate{}} : cell->second;
    }
    if (name == "runtime.mem.copy" || name == "runtime.mem.set" || name == "runtime.mem.compare") {
        if (arguments.size() != 3) throw std::runtime_error(name + " expects three arguments");
        auto getAllocation = [&](const E& expression) {
            Value value = evaluate(expression);
            auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
            if (!pointer || !*pointer || !(*pointer)->live || !(*pointer)->data) throw std::runtime_error(name + " requires allocated pointers in the interpreter");
            return *pointer;
        };
        auto first = getAllocation(arguments[0]);
        auto sizeValue = evaluate(arguments[2]); auto sizeInteger = std::get_if<std::int64_t>(&sizeValue);
        if (!sizeInteger || *sizeInteger < 0) throw std::runtime_error("memory size cannot be negative");
        auto size = static_cast<std::size_t>(*sizeInteger);
        if (size > first->size) throw std::runtime_error("memory operation exceeds allocation bounds");
        if (name == "runtime.mem.set") {
            auto value = evaluate(arguments[1]); auto integer = std::get_if<std::int64_t>(&value);
            if (!integer) throw std::runtime_error("memory set value must be an int");
            std::memset(first->data, static_cast<unsigned char>(*integer), size);
            return std::monostate{};
        }
        auto second = getAllocation(arguments[1]);
        if (size > second->size) throw std::runtime_error("memory operation exceeds allocation bounds");
        if (name == "runtime.mem.copy") { std::memmove(first->data, second->data, size); return std::monostate{}; }
        return static_cast<std::int64_t>(std::memcmp(first->data, second->data, size));
    }
    if (name == "runtime.hw.read8" || name == "runtime.hw.read32" || name == "runtime.hw.write8" || name == "runtime.hw.write32") {
        const bool write = name == "runtime.hw.write8" || name == "runtime.hw.write32";
        const bool byte = name == "runtime.hw.read8" || name == "runtime.hw.write8";
        if (arguments.size() != (write ? 2u : 1u)) throw std::runtime_error(name + " received the wrong number of arguments");
        Value addressValue = evaluate(arguments[0]);
        auto pointer = std::get_if<std::shared_ptr<Allocation>>(&addressValue);
        if (pointer && *pointer && (*pointer)->live && (*pointer)->data) {
            std::size_t width = byte ? 1 : 4;
            if ((*pointer)->size < width) throw std::runtime_error("hardware access exceeds allocation bounds");
            if (write) {
                Value writeValue = evaluate(arguments[1]); auto integer = std::get_if<std::int64_t>(&writeValue);
                if (!integer) throw std::runtime_error("hardware write value must be int");
                if (byte) *reinterpret_cast<volatile std::uint8_t*>((*pointer)->data) = static_cast<std::uint8_t>(*integer);
                else *reinterpret_cast<volatile std::uint32_t*>((*pointer)->data) = static_cast<std::uint32_t>(*integer);
                return std::monostate{};
            }
            if (byte) return static_cast<std::int64_t>(*reinterpret_cast<volatile std::uint8_t*>((*pointer)->data));
            return static_cast<std::int64_t>(*reinterpret_cast<volatile std::int32_t*>((*pointer)->data));
        }
        auto address = std::get_if<std::int64_t>(&addressValue);
        if (!address) throw std::runtime_error("hardware access requires a pointer or integer address");
        std::lock_guard lock(*hardwareMutex);
        if (write) {
            Value writeValue = evaluate(arguments[1]); auto integer = std::get_if<std::int64_t>(&writeValue);
            if (!integer) throw std::runtime_error("hardware write value must be int");
            auto mask = byte ? 0xffLL : 0xffffffffLL;
            (*hardwareRegisters)[*address] = *integer & mask;
            return std::monostate{};
        }
        auto value = (*hardwareRegisters)[*address];
        if (byte) value &= 0xff;
        else value = static_cast<std::int32_t>(value);
        return value;
    }
    if (name == "syscall.invoke3" || name == "syscall.invoke6") {
#if defined(__linux__)
        const std::size_t expected = name == "syscall.invoke3" ? 4 : 7;
        if (arguments.size() != expected) throw std::runtime_error(name + " received the wrong number of arguments");
        std::vector<std::int64_t> values; values.reserve(expected);
        for (const auto& argument : arguments) values.push_back(syscallWord(evaluate(argument)));
        long result = name == "syscall.invoke3" ? ::syscall(values[0], values[1], values[2], values[3]) : ::syscall(values[0], values[1], values[2], values[3], values[4], values[5], values[6]);
        return result < 0 ? static_cast<std::int64_t>(-errno) : static_cast<std::int64_t>(result);
#else
        throw std::runtime_error("raw syscalls are supported only on Linux");
#endif
    }
    if (name == "flt.format") {
        if (arguments.size() != 2) throw std::runtime_error("flt.format expects a number and decimal count");
        auto value = evaluate(arguments[0]);
        auto decimals = evaluate(arguments[1]);
        auto count = std::get_if<std::int64_t>(&decimals);
        if (!count) throw std::runtime_error("flt.format decimal count must be an integer");
        return runtime::floatFormat(number(value), *count);
    }
    if (name == "process.run") {
        if (arguments.size() != 1) throw std::runtime_error("process.run expects one command");
        auto commandValue = evaluate(arguments.front()); auto command = std::get_if<std::string>(&commandValue);
        if (!command) throw std::runtime_error("process.run command must be a string");
        return runtime::runProcess(*command);
    }
    if (name == "arg.count") return static_cast<std::int64_t>(this->arguments.size());
    if (name == "arg.get") {
        if (arguments.size() != 1) throw std::runtime_error("arg.get expects one index");
        auto indexValue = evaluate(arguments.front()); auto index = std::get_if<std::int64_t>(&indexValue);
        if (!index || *index < 0 || static_cast<std::uint64_t>(*index) >= this->arguments.size()) throw std::runtime_error("argument index is out of range");
        return this->arguments[static_cast<std::size_t>(*index)];
    }
    if (name == "file.read") {
        if (arguments.size() != 1) throw std::runtime_error("file.read expects one path");
        auto pathValue = evaluate(arguments.front());
        auto path = std::get_if<std::string>(&pathValue);
        if (!path) throw std::runtime_error("file.read path must be a string");
        std::ifstream file(*path, std::ios::binary);
        if (!file) throw std::runtime_error("cannot open file for reading: " + *path);
        std::ostringstream contents; contents << file.rdbuf();
        if (!file.good() && !file.eof()) throw std::runtime_error("failed while reading file: " + *path);
        return contents.str();
    }
    if (name == "file.write") {
        if (arguments.size() != 2) throw std::runtime_error("file.write expects a path and contents");
        auto pathValue = evaluate(arguments[0]); auto contentsValue = evaluate(arguments[1]);
        auto path = std::get_if<std::string>(&pathValue); auto contents = std::get_if<std::string>(&contentsValue);
        if (!path || !contents) throw std::runtime_error("file.write expects string arguments");
        std::ofstream file(*path, std::ios::binary);
        if (!file) throw std::runtime_error("cannot open file for writing: " + *path);
        file.write(contents->data(), static_cast<std::streamsize>(contents->size()));
        if (!file) throw std::runtime_error("failed while writing file: " + *path);
        return std::monostate{};
    }
    if (name == "str.length") {
        if (arguments.size() != 1) throw std::runtime_error("str.length expects one string");
        auto value = evaluate(arguments.front()); auto string = std::get_if<std::string>(&value);
        if (!string) throw std::runtime_error("str.length expects a string");
        return static_cast<std::int64_t>(string->size());
    }
    if (name == "str.builder") {
        if (arguments.size() > 1) throw std::runtime_error("str.builder expects zero or one string argument");
        if (arguments.empty()) return std::string{};
        auto value = evaluate(arguments.front());
        auto string = std::get_if<std::string>(&value);
        if (!string) throw std::runtime_error("str.builder expects a string argument");
        return *string;
    }
    if (name == "str.append") {
        if (arguments.size() != 2) throw std::runtime_error("str.append expects two strings");
        auto leftValue = evaluate(arguments[0]);
        auto rightValue = evaluate(arguments[1]);
        auto left = std::get_if<std::string>(&leftValue);
        auto right = std::get_if<std::string>(&rightValue);
        if (!left || !right) throw std::runtime_error("str.append expects string arguments");
        left->append(*right);
        return *left;
    }
    if (name == "str.find") {
        if (arguments.size() != 2) throw std::runtime_error("str.find expects two strings");
        auto sourceValue = evaluate(arguments[0]); auto needleValue = evaluate(arguments[1]);
        auto source = std::get_if<std::string>(&sourceValue); auto needle = std::get_if<std::string>(&needleValue);
        if (!source || !needle) throw std::runtime_error("str.find expects two strings");
        auto found = source->find(*needle);
        return found == std::string::npos ? std::int64_t{-1} : static_cast<std::int64_t>(found);
    }
    if (name == "str.number") {
        if (arguments.size() != 1) throw std::runtime_error("str.number expects one integer");
        auto value = evaluate(arguments.front()); auto integer = std::get_if<std::int64_t>(&value);
        if (!integer) throw std::runtime_error("str.number expects one integer");
        return std::to_string(*integer);
    }
    if (name == "str.fromchar") {
        if (arguments.size() != 1) throw std::runtime_error("str.fromchar expects one integer");
        auto value = evaluate(arguments.front()); auto integer = std::get_if<std::int64_t>(&value);
        if (!integer || *integer < 0 || *integer > 255) throw std::runtime_error("str.fromchar value must be between 0 and 255");
        return std::string(1, static_cast<char>(*integer));
    }
    if (name == "str.char") {
        if (arguments.size() != 2) throw std::runtime_error("str.char expects a string and index");
        auto stringValue = evaluate(arguments[0]); auto indexValue = evaluate(arguments[1]);
        auto string = std::get_if<std::string>(&stringValue); auto index = std::get_if<std::int64_t>(&indexValue);
        if (!string || !index || *index < 0 || static_cast<std::uint64_t>(*index) >= string->size()) throw std::runtime_error("str.char index is out of range");
        return static_cast<std::int64_t>(static_cast<unsigned char>((*string)[static_cast<std::size_t>(*index)]));
    }
    if (name == "str.slice") {
        if (arguments.size() != 3) throw std::runtime_error("str.slice expects a string and two bounds");
        auto stringValue = evaluate(arguments[0]); auto firstValue = evaluate(arguments[1]); auto lastValue = evaluate(arguments[2]);
        auto string = std::get_if<std::string>(&stringValue); auto first = std::get_if<std::int64_t>(&firstValue); auto last = std::get_if<std::int64_t>(&lastValue);
        if (!string || !first || !last || *first < 0 || *last < *first || static_cast<std::uint64_t>(*last) > string->size()) throw std::runtime_error("str.slice bounds are out of range");
        return string->substr(static_cast<std::size_t>(*first), static_cast<std::size_t>(*last - *first));
    }
    if (name == "alloc" || name == "alloc.aligned" || name == "new" || name == "aligned_alloc") {
        const bool aligned = name == "alloc.aligned" || name == "aligned_alloc";
        const std::size_t expected = aligned ? 2 : 1;
        if (arguments.size() != expected) throw std::runtime_error(name + " expects " + std::to_string(expected) + " argument(s)");
        auto requested = evaluate(arguments[0]);
        auto integer = std::get_if<std::int64_t>(&requested);
        if (!integer || *integer < 0 || static_cast<std::uint64_t>(*integer) > std::numeric_limits<std::size_t>::max()) throw std::runtime_error("allocation size is out of range");
        auto size = static_cast<std::size_t>(*integer);
        std::size_t alignment = alignof(std::max_align_t);
        if (aligned) {
            auto requestedAlignment = evaluate(arguments[1]);
            auto alignmentValue = std::get_if<std::int64_t>(&requestedAlignment);
            if (!alignmentValue || *alignmentValue < static_cast<std::int64_t>(alignof(void*)) ||
                static_cast<std::uint64_t>(*alignmentValue) > std::numeric_limits<std::size_t>::max())
                throw std::runtime_error("alignment must be a power of two at least alignof(void*)");
            alignment = static_cast<std::size_t>(*alignmentValue);
            if ((alignment & (alignment - 1)) != 0) throw std::runtime_error("alignment must be a power of two at least alignof(void*)");
        }
        auto allocation = std::make_shared<Allocation>(); allocation->size = size; allocation->alignment = alignment;
        if (!regions.empty()) {
            if (!regions.back().arena) regions.back().arena = std::make_shared<AllocationArena>();
            allocation->data = regions.back().arena->allocate(size, alignment);
            regions.back().allocations.push_back(allocation);
        } else {
            if (aligned) {
                allocation->alignedOwned = ::operator new(size == 0 ? 1 : size, std::align_val_t(alignment));
                allocation->data = static_cast<std::byte*>(allocation->alignedOwned);
            } else {
                allocation->owned = std::make_unique<std::byte[]>(size);
                allocation->data = allocation->owned.get();
            }
            manualAllocations.push_back(allocation);
        }
        return allocation;
    }
    if (name == "free" || name == "delete") {
        if (arguments.size() != 1) throw std::runtime_error("free expects one pointer argument");
        auto value = evaluate(arguments[0]); auto pointer = std::get_if<std::shared_ptr<Allocation>>(&value);
        if (!pointer && !std::holds_alternative<std::monostate>(value)) throw std::runtime_error("free requires an allocated pointer");
        if (pointer && *pointer) {
            (*pointer)->live = false;
            (*pointer)->owned.reset();
            if ((*pointer)->alignedOwned) {
                ::operator delete((*pointer)->alignedOwned, std::align_val_t((*pointer)->alignment));
                (*pointer)->alignedOwned = nullptr;
            }
            (*pointer)->data = nullptr;
            (*pointer)->size = 0;
            (*pointer)->cell = std::monostate{};
            (*pointer)->cellSet = false;
            manualAllocations.erase(std::remove(manualAllocations.begin(), manualAllocations.end(), *pointer), manualAllocations.end());
        }
        assign(arguments[0], std::monostate{});
        return std::monostate{};
    }
    if (name == "write" || name == "writeln") {
        std::vector<Value> values; values.reserve(arguments.size());
        for (const auto& argument : arguments) values.push_back(evaluate(argument));
        std::lock_guard lock(*ioMutex);
        for (const auto& value : values) printValue(output, value);
        if (name == "writeln") output << '\n';
        return std::monostate{};
    }
    if (name == "read" || name == "readln") {
        if (arguments.size() != 1 || !std::dynamic_pointer_cast<Name>(arguments[0])) throw std::runtime_error(name + " expects a variable name");
        std::lock_guard ioLock(*ioMutex);
        const auto targetName = std::dynamic_pointer_cast<Name>(arguments[0])->value;
        if (name == "read") {
            std::string targetType;
            for (std::size_t n = types.size(); n > 0; --n) { auto found = types[n - 1].find(targetName); if (found != types[n - 1].end()) { targetType = found->second; break; } }
            if (targetType == "int") { std::int64_t value; if (!(input >> value)) throw std::runtime_error("failed to read integer"); const int next = input.peek(); if (next != std::char_traits<char>::eof() && std::isspace(static_cast<unsigned char>(next))) input.get(); assign(arguments[0], value); }
            else { double value; if (!(input >> value)) throw std::runtime_error("failed to read number"); const int next = input.peek(); if (next != std::char_traits<char>::eof() && std::isspace(static_cast<unsigned char>(next))) input.get(); assign(arguments[0], value); }
        }
        else { std::string value; if (!std::getline(input, value)) throw std::runtime_error("failed to read line"); assign(arguments[0], value); }
        return std::monostate{};
    }
    auto function = functions.find(name);
    if (function == functions.end()) throw std::runtime_error("unknown function: " + name);
    const Function& target = *function->second;
    if (arguments.size() != target.params.size()) throw std::runtime_error("wrong number of arguments to " + name);
    std::vector<Value> values; values.reserve(arguments.size());
    for (const auto& argument : arguments) values.push_back(evaluate(argument));
    scopes.emplace_back(); types.emplace_back(); deferredActions.emplace_back();
    for (std::size_t i = 0; i < values.size(); ++i) { scopes.back()[target.params[i].name] = coerceValue(std::move(values[i]), target.params[i].type); types.back()[target.params[i].name] = target.params[i].type; }
    try {
        executeBlock(target.body, false);
        runDeferred(deferredActions.size() - 1);
        scopes.pop_back(); types.pop_back(); deferredActions.pop_back();
        return std::monostate{};
    } catch (const ReturnSignal& signal) {
        try { runDeferred(deferredActions.size() - 1); }
        catch (...) { scopes.pop_back(); types.pop_back(); deferredActions.pop_back(); throw; }
        scopes.pop_back(); types.pop_back(); deferredActions.pop_back();
        return coerceValue(signal.value, target.result);
    } catch (...) {
        auto error = std::current_exception();
        try { runDeferred(deferredActions.size() - 1); } catch (...) {}
        scopes.pop_back(); types.pop_back(); deferredActions.pop_back();
        std::rethrow_exception(error);
    }
}
void Evaluator::executeBlock(const std::shared_ptr<Block>& body, bool newScope) {
    if (newScope) { scopes.emplace_back(); types.emplace_back(); deferredActions.emplace_back(); }
    try { for (const auto& statement : body->items) execute(statement); }
    catch (...) {
        auto error = std::current_exception();
        if (newScope) {
            try { runDeferred(deferredActions.size() - 1); } catch (...) {}
            scopes.pop_back(); types.pop_back(); deferredActions.pop_back();
        }
        std::rethrow_exception(error);
    }
    if (newScope) {
        try { runDeferred(deferredActions.size() - 1); }
        catch (...) { scopes.pop_back(); types.pop_back(); deferredActions.pop_back(); throw; }
        scopes.pop_back(); types.pop_back(); deferredActions.pop_back();
    }
}
void Evaluator::runDeferred(std::size_t scopeIndex) {
    auto actions = std::move(deferredActions.at(scopeIndex));
    deferredActions[scopeIndex].clear();
    std::exception_ptr firstError;
    for (auto action = actions.rbegin(); action != actions.rend(); ++action) {
        try { evaluate(*action); }
        catch (...) { if (!firstError) firstError = std::current_exception(); }
    }
    if (firstError) std::rethrow_exception(firstError);
}
void Evaluator::execute(const S& statement) {
    if (auto variable = std::dynamic_pointer_cast<Var>(statement)) {
        Value value = variable->value ? coerceValue(evaluate(variable->value), variable->type) : defaultValue(variable->type);
        scopes.back()[variable->name] = std::move(value); types.back()[variable->name] = variable->type; return;
    }
    if (auto assignment = std::dynamic_pointer_cast<Assign>(statement)) { assign(assignment->target, evaluate(assignment->value)); return; }
    if (auto call = std::dynamic_pointer_cast<ExprStmt>(statement)) { evaluate(call->value); return; }
    if (auto deferred = std::dynamic_pointer_cast<Defer>(statement)) { deferredActions.back().push_back(deferred->action); return; }
    if (auto ret = std::dynamic_pointer_cast<Return>(statement)) throw ReturnSignal{ret->value ? evaluate(ret->value) : Value(std::monostate{})};
    if (auto gc = std::dynamic_pointer_cast<Gc>(statement)) {
        regions.push_back({std::make_shared<AllocationArena>(), {}});
        auto releaseRegion = [&] {
            for (auto& weak : regions.back().allocations) if (auto allocation = weak.lock()) {
                allocation->live = false;
                allocation->data = nullptr;
                allocation->size = 0;
                allocation->owned.reset();
                if (allocation->alignedOwned) {
                    ::operator delete(allocation->alignedOwned, std::align_val_t(allocation->alignment));
                    allocation->alignedOwned = nullptr;
                }
                allocation->cell = std::monostate{};
                allocation->cellSet = false;
            }
            regions.pop_back();
        };
        try { executeBlock(gc->body); }
        catch (...) { releaseRegion(); throw; }
        releaseRegion();
        return;
    }
    if (auto assembly = std::dynamic_pointer_cast<Asm>(statement)) {
        if (std::any_of(assembly->text.begin(), assembly->text.end(), [](unsigned char value) { return !std::isspace(value); })) executeInlineAssembly(assembly->text);
        return;
    }
    if (auto branch = std::dynamic_pointer_cast<If>(statement)) { if (truth(evaluate(branch->condition))) executeBlock(branch->yes); else if (branch->no) executeBlock(branch->no); return; }
    if (auto loop = std::dynamic_pointer_cast<While>(statement)) { while (truth(evaluate(loop->condition))) executeBlock(loop->body); return; }
    if (auto selection = std::dynamic_pointer_cast<Switch>(statement)) {
        Value selected = evaluate(selection->value);
        for (const auto& arm : selection->cases) { if (number(evaluate(arm.value)) == number(selected)) { executeBlock(arm.body); return; } }
        if (selection->fallback) executeBlock(selection->fallback);
        return;
    }
    if (auto nested = std::dynamic_pointer_cast<Block>(statement)) { executeBlock(nested); return; }
    throw std::runtime_error("unsupported statement");
}
int Evaluator::run(const Program& program) {
    functions.clear(); recordTypes.clear(); scopes.clear(); types.clear(); deferredActions.clear(); regions.clear(); manualAllocations.clear(); { std::lock_guard lock(*hardwareMutex); hardwareRegisters->clear(); }
    for (const auto& record : program.records) recordTypes.emplace(record.name, &record);
    for (const auto& function : program.functions) functions.emplace(function.name, &function);
    auto main = functions.find("main"); if (main == functions.end()) throw std::runtime_error("no main function");
    Value result = invoke("main", {});
    if (auto integer = std::get_if<std::int64_t>(&result)) return static_cast<int>(*integer);
    if (auto real = std::get_if<double>(&result)) return static_cast<int>(*real);
    return 0;
}
}
