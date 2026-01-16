#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "SpatialNode.hpp"       
#include "SiteNode.hpp"          
#include "BuildingNode.hpp"      
#include "StoryNode.hpp"         
#include "SpaceNode.hpp"         
#include "ElementNode.hpp" 

std::string getIfcName(const Entity& e);


class SpatialTree {
public:
    std::unique_ptr<SiteNode> root;

    void build(const std::vector<std::unique_ptr<Entity>>& entities);
    void print() const;

private:
    // Node pools (pre-wiring)
    std::unordered_map<std::string, std::unique_ptr<BuildingNode>> buildingPool_;
    std::unordered_map<std::string, std::unique_ptr<StoryNode>>    storyPool_;
    std::unordered_map<std::string, std::unique_ptr<SpaceNode>>    spacePool_;

    // Central element ownership: unique per element-id
    std::unordered_map<std::string, std::unique_ptr<ElementNode>>  elementPool_;

    // Fast lookups after wiring
    std::unordered_map<std::string, BuildingNode*> buildingById_;
    std::unordered_map<std::string, StoryNode*>    storyById_;
    std::unordered_map<std::string, SpaceNode*>    spaceById_;
    std::unordered_map<std::string, ElementNode*>  elementById_;

    // internal helper
    template <class T>
    static std::unique_ptr<T> take(std::unordered_map<std::string, std::unique_ptr<T>>& m,
                                   const std::string& key) {
        auto it = m.find(key);
        if (it == m.end()) return nullptr;
        auto ptr = std::move(it->second);
        m.erase(it);
        return ptr;
    }

    void reset_state();
    static bool isDesiredElementType(const std::string& ifcType);

    void processAggregates(const std::vector<const Entity*>& rels);
    void processContained(const std::vector<const Entity*>& rels,
                          const std::unordered_map<std::string, const Entity*>& entById);
    void processBoundaries(const std::vector<const Entity*>& rels,
                           const std::unordered_map<std::string, const Entity*>& entById);
};
