#include "StoryNode.hpp"
#include <iostream>
#include <algorithm>

StoryNode::StoryNode(Entity* e, const std::string& id, const std::string& name)
    : SpatialNode(e, id, name) {
}

void StoryNode::addSpace(std::unique_ptr<SpaceNode> space) {
    if (space) {
        space->parent = this;
        spaces.push_back(std::move(space));
    }
}

void StoryNode::addElement(ElementNode* element) {
    if (element) {
        if (std::find(elements.begin(), elements.end(), element) == elements.end()) {
            elements.push_back(element);
        }
    }
}

void StoryNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };

    printIndent(indent);
    std::cout << "STORY [" << id << "] " << name;

    if (!elements.empty()) {
        std::cout << " (Elements: " << elements.size() << ")";
    }
    std::cout << std::endl;
    
    for (const auto& space : spaces) {
        space->print(indent + 1);
    }
}