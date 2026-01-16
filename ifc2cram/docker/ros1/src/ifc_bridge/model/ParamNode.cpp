#include "ParamNode.hpp"

ParamNode::ParamNode(const std::string& val) : value(val) {}

void ParamNode::addChild(std::unique_ptr<ParamNode> child) {
    children.push_back(std::move(child));
}

void ParamNode::print(int indent) const {
    for (int i = 0; i < indent; i++) std::cout << "  ";
    std::cout << value << "\n";
    for (const auto& child : children) {
        child->print(indent + 1);
    }
}

const ParamNode* ParamNode::find(const std::string& key) const {
    if (value == key) return this;
    for (const auto& child : children) {
        if (const auto* res = child->find(key)) return res;
    }
    return nullptr;
}
