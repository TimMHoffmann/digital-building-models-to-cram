#ifndef SPACEBOUNDARYNODE_HPP
#define SPACEBOUNDARYNODE_HPP

#include "SpatialNode.hpp"
#include <array>
#include <vector>
#include "ElementNode.hpp"


class SpaceBoundaryNode : public SpatialNode {
public:
    ElementNode* relatedElement = nullptr;
    std::vector<std::array<double, 3>> boundaryPoints;

    SpaceBoundaryNode(Entity* e, const std::string& id, const std::string& name);
    void print(int indent = 0) const override;
};

#endif // SPACEBOUNDARYNODE_HPP