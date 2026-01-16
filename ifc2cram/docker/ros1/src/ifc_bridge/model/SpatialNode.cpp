#include "SpatialNode.hpp"
#include "ElementNode.hpp" 
#include "SpaceBoundaryNode.hpp"
#include "SpaceNode.hpp"
#include "StoryNode.hpp"
#include "BuildingNode.hpp"
#include "SiteNode.hpp"

#include <iostream>
#include <iomanip>
#include <typeinfo>

using namespace std;

static void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) {
        std::cout << "  ";
    }
}

SpatialNode::SpatialNode(Entity* e, std::string i, std::string n)
    : id(std::move(i)), name(std::move(n)), entity(e) {
}

Matrix4x4 SpatialNode::getGlobalPlacement() const {
    return localPlacement;
}

Matrix4x4 SpatialNode::getAbsolutePlacement() const {
    if (parent) {
        return parent->getAbsolutePlacement() * localPlacement;
    }
    return localPlacement; 
}

void SpatialNode::print(int indent) const {
    printIndent(indent);
    std::cout << "[" << id << "] " << name << " (" << typeid(*this).name() << ")" << std::endl;
}