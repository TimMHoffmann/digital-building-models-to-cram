#ifndef SPACENODE_HPP
#define SPACENODE_HPP

#include "SpatialNode.hpp"
#include <vector>
#include <memory>
#include "ElementNode.hpp"
#include "SpaceBoundaryNode.hpp"
#include "RobotTask.hpp"

class SpaceNode : public SpatialNode {
public:
    std::vector<ElementNode*> elements;
    std::vector<std::unique_ptr<SpaceBoundaryNode>> boundaries;
    std::vector<std::string> connected_space_ids;
    std::vector<RobotTask> robotTasks;
    SpaceNode(Entity* e, const std::string& id, const std::string& name);

     void addConnectedSpace(const std::string& other_id) {
        if (other_id.empty()) return;
        for (const auto& e : connected_space_ids) if (e == other_id) return;
        connected_space_ids.push_back(other_id);
    }

    void addElement(ElementNode* element);
    void addBoundary(std::unique_ptr<SpaceBoundaryNode> boundary);

    void print(int indent = 0) const override;
};

#endif // SPACENODE_HPP