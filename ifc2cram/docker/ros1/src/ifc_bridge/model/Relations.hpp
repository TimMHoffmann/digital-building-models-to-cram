#pragma once
#include <string>
#include <vector>
#include <optional>
#include "Entity.hpp"


bool isRelAggregates(const Entity& e);
bool isRelContainedInSpatialStructure(const Entity& e);
bool isRelSpaceBoundary(const Entity& e);

std::optional<std::string> getRelatingId(const Entity& e);
std::vector<std::string>   getRelatedIds(const Entity& e);
