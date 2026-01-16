#include "Entity.hpp"
#include <iostream>

Entity::Entity(std::string id, std::string type)
    : id(std::move(id)), type(std::move(type)) {}

Entity::Entity(std::string id, std::string type,
               std::vector<std::unique_ptr<ParamNode>> params)
    : id(std::move(id)), type(std::move(type)), params(std::move(params)) {}

void Entity::addParam(std::unique_ptr<ParamNode> param) {
    params.push_back(std::move(param));
}

const ParamNode* Entity::findParam(const std::string& key) const {
    for (const auto& p : params) {
        if (p->value == key) return p.get();
    }
    return nullptr;
}

void Entity::printParams() const {
    std::cout << "Entity " << id << " (" << type << ")\n";
    for (const auto& p : params) {
        p->print(1);
    }
}
