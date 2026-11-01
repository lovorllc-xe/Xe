#pragma once
#include "ast.hpp"
#include <string>
namespace xe {
class Codegen {
public:
    std::string generate(const Program& program);
private:
    std::size_t expressionScopeId = 0;
    void block(const std::shared_ptr<Block>& body, std::string& output, int indent);
    std::string expression(const E& expression);
    bool isLvalue(const E& expression) const;
    std::string type(const std::string& type) const;
    std::string defaultValue(const std::string& type) const;
    std::string name(const std::string& name) const;
    std::string quote(const std::string& value) const;
};
}
