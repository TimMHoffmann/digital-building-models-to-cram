#ifndef BUILDINGNODE_HPP
#define BUILDINGNODE_HPP

#include "SpatialNode.hpp"
#include <vector>
#include <memory>
#include "StoryNode.hpp"

class BuildingNode : public SpatialNode {
public:
    std::vector<std::unique_ptr<StoryNode>> stories;

    BuildingNode(Entity* e, const std::string& id, const std::string& name);
    void addStory(std::unique_ptr<StoryNode> story);
    void print(int indent = 0) const override;
};

#endif 