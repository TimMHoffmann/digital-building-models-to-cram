#ifndef SPATIALNODE_HPP
#define SPATIALNODE_HPP

#include <string>
#include <memory>
#include "Entity.hpp"
#include "Matrix4x4.hpp" 

class SpaceBoundaryNode;
class ElementNode;

class SpatialNode {
public:
    std::string id;
    std::string name;
    Entity* entity;

    Matrix4x4 localPlacement; 
    SpatialNode* parent = nullptr; 
    
    explicit SpatialNode(Entity* e, std::string id, std::string name);
    virtual ~SpatialNode() = default;

    virtual Matrix4x4 getGlobalPlacement() const;
    virtual Matrix4x4 getAbsolutePlacement() const;
    virtual void print(int indent = 0) const;
};

#endif // SPATIALNODE_HPP