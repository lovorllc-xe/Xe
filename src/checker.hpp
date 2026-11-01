#pragma once
#include "ast.hpp"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
namespace xe {
class Checker {
public:
    void check(const Program& program);
private:
    struct Symbol {
        std::string type;
        int regionDepth;
        int pointerRegionDepth = -1;
        int scopeDepth = 0;
        int pointerScopeDepth = -1;
        bool heapOwned = false;
        bool immutable = false;
    };
    std::unordered_map<std::string, const Function*> functions;
    std::unordered_map<std::string, const RecordType*> records;
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int regionDepth = 0;
    std::string currentResult;
    void checkBlock(const std::shared_ptr<Block>& block, bool newScope = true);
    void checkStatement(const S& statement);
    void checkStatementImpl(const S& statement);
    std::string checkExpression(const E& expression);
    const Symbol* findSymbol(const std::string& name) const;
    const Symbol* storageSymbol(const E& expression) const;
    int pointerRegion(const E& expression) const;
    int pointerScope(const E& expression) const;
    bool heapOwnedPointer(const E& expression) const;
    bool functionReturnsOwnedPointer(const std::string& name, std::unordered_set<std::string>& active) const;
    bool functionMayRetainParameter(const std::string& name, std::size_t parameterIndex) const;
    bool addressOfRegionValue(const E& expression, int outerScopeDepth, int outerRegionDepth) const;
    void checkType(const std::string& type) const;
};
}
