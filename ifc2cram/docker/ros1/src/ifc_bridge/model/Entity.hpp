#ifndef ENTITY_HPP
#define ENTITY_HPP

#include <string>
#include <vector>
#include <memory>
#include "ParamNode.hpp"

class Entity {
public:
    std::string id;
    std::string type;
    std::vector<std::unique_ptr<ParamNode>> params;

    Entity(std::string id, std::string type);

    Entity(std::string id, std::string type,
           std::vector<std::unique_ptr<ParamNode>> params);

    void addParam(std::unique_ptr<ParamNode> param);
    const ParamNode* findParam(const std::string& key) const;
    void printParams() const;
};

#endif
