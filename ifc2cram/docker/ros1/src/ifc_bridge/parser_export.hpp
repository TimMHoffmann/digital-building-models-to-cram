#pragma once
#include <vector>
#include <memory>
#include "model/Entity.hpp"

extern std::vector<std::unique_ptr<Entity>> entities; 

const std::vector<std::unique_ptr<Entity>>& getParsedEntities() {
    return entities;
}