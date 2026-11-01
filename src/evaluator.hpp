#pragma once
#include "ast.hpp"
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace xe {
struct RuntimeArray;
struct RuntimeRecord;
struct AllocationArena;
struct Allocation;
struct RuntimeReference;
using Value = std::variant<std::monostate, std::int64_t, double, std::string,
                           std::shared_ptr<RuntimeArray>, std::shared_ptr<RuntimeRecord>, std::shared_ptr<Allocation>, std::shared_ptr<RuntimeReference>>;
struct RuntimeArray { std::vector<Value> values; };
struct RuntimeRecord { std::unordered_map<std::string, Value> fields; };
struct Allocation {
    std::unique_ptr<std::byte[]> owned;
    void* alignedOwned = nullptr;
    std::size_t alignment = alignof(std::max_align_t);
    std::byte* data = nullptr;
    std::size_t size = 0;
    std::size_t byteOffset = 0;
    std::shared_ptr<Allocation> backing;
    std::unordered_map<std::size_t, Value> cells;
    bool live = true;
    Value cell;
    bool cellSet = false;
    ~Allocation();
};
struct AllocationRegion {
    std::shared_ptr<AllocationArena> arena;
    std::vector<std::weak_ptr<Allocation>> allocations;
};
struct RuntimeReference { std::size_t scopeIndex = 0; std::string name, type; std::shared_ptr<RuntimeArray> array; std::size_t index = 0; bool arrayElement = false; };

class Evaluator {
public:
    Evaluator(std::istream& input, std::ostream& output, std::vector<std::string> arguments = {});
    int run(const Program& program);
private:
    std::istream& input;
    std::ostream& output;
    std::vector<std::string> arguments;
    std::unordered_map<std::string, const Function*> functions;
    std::unordered_map<std::string, const RecordType*> recordTypes;
    std::vector<std::unordered_map<std::string, Value>> scopes;
    std::vector<std::unordered_map<std::string, std::string>> types;
    std::vector<std::vector<E>> deferredActions;
    std::vector<AllocationRegion> regions;
    std::vector<std::shared_ptr<Allocation>> manualAllocations;
    std::shared_ptr<std::unordered_map<std::int64_t, std::int64_t>> hardwareRegisters = std::make_shared<std::unordered_map<std::int64_t, std::int64_t>>();
    std::shared_ptr<std::mutex> hardwareMutex = std::make_shared<std::mutex>();
    std::shared_ptr<std::mutex> ioMutex = std::make_shared<std::mutex>();
    Value evaluate(const E& expression);
    Value defaultValue(const std::string& type);
    std::string expressionType(const E& expression) const;
    std::int64_t hardwareValue(const Value& value) const;
    void storeHardwareValue(Value& target, std::int64_t value);
    Value addressOf(const E& expression);
    Value invoke(const std::string& name, const std::vector<E>& arguments);
    void execute(const S& statement);
    void executeBlock(const std::shared_ptr<Block>& block, bool newScope = true);
    void runDeferred(std::size_t scopeIndex);
    Value lookup(const std::string& name) const;
    void assign(const E& target, Value value);
};
}
