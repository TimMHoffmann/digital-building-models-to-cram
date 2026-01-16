#include "ElementNode.hpp"
#include "SpaceBoundaryNode.hpp" // Für SpaceBoundaryNode*
#include <iostream>
#include <algorithm>
#include <iomanip> 
#include "Matrix4x4.hpp"   

ElementNode::ElementNode(Entity* e, const std::string& id, const std::string& name, const std::string& ifcType)
    : SpatialNode(e, id, name), m_ifcType(ifcType) {
}

void ElementNode::addBoundary(SpaceBoundaryNode* boundary) {
    if (boundary) {
        if (std::find(boundaries.begin(), boundaries.end(), boundary) == boundaries.end()) {
            boundaries.push_back(boundary);
        }
    }
}

void ElementNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };
    
    const Matrix4x4 absolutePlacement = this->getAbsolutePlacement();
    
    double tx = absolutePlacement.m[0][3];
    double ty = absolutePlacement.m[1][3];
    double tz = absolutePlacement.m[2][3];


    printIndent(indent);
    std::cout << "ELEMENT [" << id << "] " << name;
    
    std::cout << std::fixed << std::setprecision(2); 
    std::cout << " (Pos: X=" << tx << ", Y=" << ty << ", Z=" << tz << ")";
    
    std::cout << " (Boundaries: " << boundaries.size() << ")";
    std::cout << std::endl;
}
