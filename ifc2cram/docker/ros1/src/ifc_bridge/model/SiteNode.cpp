#include "SiteNode.hpp"
#include <iostream>

SiteNode::SiteNode(Entity* e, const std::string& id, const std::string& name)
    : SpatialNode(e, id, name) {
}

void SiteNode::addBuilding(std::unique_ptr<BuildingNode> building) {
    if (building) {
        building->parent = this;
        buildings.push_back(std::move(building));
    }
}

void SiteNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };

    printIndent(indent);
    std::cout << "SITE [" << id << "] " << name << std::endl;
    for (const auto& building : buildings) {
        building->print(indent + 1);
    }
}