#ifndef PARAMNODE_HPP
#define PARAMNODE_HPP

#include <string>
#include <vector>
#include <memory>
#include <iostream>

class ParamNode {
public:
    std::string value;
    std::vector<std::unique_ptr<ParamNode>> children;

    explicit ParamNode(const std::string& val);
    void addChild(std::unique_ptr<ParamNode> child);
    void print(int indent = 0) const;
    const ParamNode* find(const std::string& key) const;
};

#endif
