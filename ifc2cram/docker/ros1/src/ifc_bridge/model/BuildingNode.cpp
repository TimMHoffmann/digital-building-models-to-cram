#include "BuildingNode.hpp"
#include <iostream>

BuildingNode::BuildingNode(Entity* e, const std::string& id, const std::string& name)
    : SpatialNode(e, id, name) {
}

void BuildingNode::addStory(std::unique_ptr<StoryNode> story) {
    if (story) {
        story->parent = this;
        stories.push_back(std::move(story));
    }
}

void BuildingNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };

    printIndent(indent);
    std::cout << "BUILDING [" << id << "] " << name << std::endl;
    for (const auto& story : stories) {
        story->print(indent + 1);
    }
}