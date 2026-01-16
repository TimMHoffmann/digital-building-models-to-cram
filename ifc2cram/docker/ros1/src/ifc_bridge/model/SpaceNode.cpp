#include "SpaceNode.hpp"
#include <iostream>
#include <algorithm>

SpaceNode::SpaceNode(Entity* e, const std::string& id, const std::string& name)
    : SpatialNode(e, id, name) {
}

void SpaceNode::addElement(ElementNode* element) {
    if (element) {
        if (std::find(elements.begin(), elements.end(), element) == elements.end()) {
            elements.push_back(element);
        }
    }
}

void SpaceNode::addBoundary(std::unique_ptr<SpaceBoundaryNode> boundary) {
    if (boundary) {
        boundary->parent = this; 
        boundaries.push_back(std::move(boundary));
    }
}

void SpaceNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };

    printIndent(indent);
    std::cout << "SPACE [" << id << "] " << name << std::endl;

    for (const auto& boundary : boundaries) {
        boundary->print(indent + 1);
    }

    if (!elements.empty()) {
        printIndent(indent + 1);
        std::cout << "Elements in Space (" << elements.size() << "):" << std::endl;
        for (const auto& element : elements) {
            printIndent(indent + 2);
            std::cout << "* " << element->id << " (" << element->name << ")" << std::endl;
        }
    }
}