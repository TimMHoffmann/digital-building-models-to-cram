#ifndef SITENODE_HPP
#define SITENODE_HPP

#include "SpatialNode.hpp"
#include <vector>
#include <memory>
#include "BuildingNode.hpp"

class SiteNode : public SpatialNode {
public:
    std::vector<std::unique_ptr<BuildingNode>> buildings;

    SiteNode(Entity* e, const std::string& id, const std::string& name);
    void addBuilding(std::unique_ptr<BuildingNode> building);
    void print(int indent = 0) const override;
};

#endif // SITENODE_HPP