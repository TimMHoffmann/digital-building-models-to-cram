#include "SpaceBoundaryNode.hpp"
#include "ElementNode.hpp" 
#include <iostream>
#include <iomanip>
#include "ParamNode.hpp"

SpaceBoundaryNode::SpaceBoundaryNode(Entity* e, const std::string& id, const std::string& name)
    : SpatialNode(e, id, name) {
}

void SpaceBoundaryNode::print(int indent) const {
    auto printIndent = [](int ind) { for (int i = 0; i < ind; ++i) std::cout << "  "; };
    
    printIndent(indent);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "BOUNDARY [" << id << "] " << name;

    if (relatedElement) {
        
        std::cout << " (Related Element: " << relatedElement->id << " - " << relatedElement->name << relatedElement->getIfcType() << ")";
    
    }
    std::cout << " (Points: " << boundaryPoints.size() << ")";
    
    
    std::cout << std::endl;

    if (!boundaryPoints.empty()) {
        const Matrix4x4 absolutePlacement = getAbsolutePlacement();
        printIndent(indent + 1);
        std::cout << "Absolute Coordinates (World Space) [mm]:" << std::endl; 
        
        for (const auto& localPoint : boundaryPoints) {
            std::array<double, 3> worldPoint = absolutePlacement.transformPoint(localPoint);
            
            worldPoint[0] *= 1000.0;
            worldPoint[1] *= 1000.0;
            worldPoint[2] *= 1000.0;
            
            printIndent(indent + 2);
            std::cout << "Local (" 
                      << localPoint[0] << ", " 
                      << localPoint[1] << ", " << localPoint[2] << ") -> "
                      << "World (" 
                      << worldPoint[0] << ", " 
                      << worldPoint[1] << ", " << worldPoint[2] << ")"
                      << std::endl;
        }
    }
}