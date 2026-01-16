#ifndef ELEMENTNODE_HPP
#define ELEMENTNODE_HPP

#include "SpatialNode.hpp"
#include <vector>

class ElementNode : public SpatialNode {
    private: 
        std::string m_ifcType;

    public:
    std::string geometryEntityId; 
    std::vector<SpaceBoundaryNode*> boundaries;

    ElementNode(Entity* e, const std::string& id, const std::string& name, const std::string& ifcType);
    void print(int indent = 0) const override;
    
    void addBoundary(SpaceBoundaryNode* boundary);
    std::string getIfcType() const { return m_ifcType; } 
};

#endif // ELEMENTNODE_HPP