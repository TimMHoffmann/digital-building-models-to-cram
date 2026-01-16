#ifndef STORYNODE_HPP
#define STORYNODE_HPP

#include "SpatialNode.hpp"
#include <vector>
#include <memory>
#include "SpaceNode.hpp"
#include "ElementNode.hpp"

class StoryNode : public SpatialNode {
public:
    std::vector<std::unique_ptr<SpaceNode>> spaces;
    std::vector<ElementNode*> elements; // Non-owning

    StoryNode(Entity* e, const std::string& id, const std::string& name);

    void addSpace(std::unique_ptr<SpaceNode> space);
    void addElement(ElementNode* element);

    void print(int indent = 0) const override;
};

#endif // STORYNODE_HPP