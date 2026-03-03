#include "SpatialTree.hpp"
#include "SpatialTreeHelpers.cpp" 
#include "Relations.hpp"
#include "RobotTask.hpp"
#include <iostream>
#include <utility>    
#include <algorithm> 
#include <iomanip>

void SpatialTree::reset_state() {
    root.reset();
    elementPool_.clear();
    buildingPool_.clear();
    storyPool_.clear();
    spacePool_.clear();
    elementById_.clear();
    buildingById_.clear();
    storyById_.clear();
    spaceById_.clear();
}


bool SpatialTree::isDesiredElementType(const std::string& type) {
    return type.find("IFCWALL") != std::string::npos || 
           type.find("IFCSLAB") != std::string::npos ||
           type.find("IFCCOLUMN") != std::string::npos ||
           type.find("IFCBEAM") != std::string::npos ||
           type.find("IFCDOOR") != std::string::npos ||
           type.find("IFCWINDOW") != std::string::npos  || 
           type.find("IFCSTAIR") != std::string::npos;
}

void SpatialTree::build(const std::vector<std::unique_ptr<Entity>>& entities) {
    reset_state();

    std::unordered_map<std::string, const Entity*> entById;
    entById.reserve(entities.size());

    std::vector<const Entity*> relAggregates;
    std::vector<const Entity*> relContained;
    std::vector<const Entity*> relBoundaries;
    std::vector<const Entity*> relProperties;

    for (const auto& e : entities) {
        if (!e) continue;
        entById[e->id] = e.get();

        const std::string& t = e->type;

        if (t == "IFCSITE") {
            if (!root) {
                root = std::make_unique<SiteNode>(e.get(), e->id, t);
                root->localPlacement = Matrix4x4();
            }
        }
        else if (t == "IFCBUILDING") {
            buildingPool_.emplace(e->id, std::make_unique<BuildingNode>(e.get(), e->id, getIfcName(*e)));
        }
        else if (t == "IFCBUILDINGSTOREY") {
            storyPool_.emplace(e->id, std::make_unique<StoryNode>(e.get(), e->id, getIfcName(*e)));
        }
        else if (t == "IFCSPACE") {
            spacePool_.emplace(e->id, std::make_unique<SpaceNode>(e.get(), e->id, getIfcName(*e)));
        }
        else if (isRelAggregates(*e)) {
            relAggregates.push_back(e.get());
        }
        else if (isRelContainedInSpatialStructure(*e)) {
            relContained.push_back(e.get());
        }
        else if (isRelSpaceBoundary(*e)) {
            relBoundaries.push_back(e.get());
        }
        else if (isRelDefinesByProperties(*e)) {
            relProperties.push_back(e.get());
        }
    }

    if (!root) {
        std::cout << "[build] No IFCSITE found – tree stays empty.\n";
        return;
    }

    for (auto& kv : buildingPool_) buildingById_[kv.first] = kv.second.get();
    for (auto& kv : storyPool_)    storyById_[kv.first]    = kv.second.get();
    for (auto& kv : spacePool_)    spaceById_[kv.first]    = kv.second.get();
    
    for (auto& [id, node] : buildingPool_) {
        if (auto mat = parseLocalPlacement(*node->entity, entById)) {
            node->localPlacement = *mat;
        }
    }
    for (auto& [id, node] : storyPool_) {
        if (auto mat = parseLocalPlacement(*node->entity, entById)) {
            node->localPlacement = *mat;
        }
    }
    for (auto& [id, node] : spacePool_) {
        if (auto mat = parseLocalPlacement(*node->entity, entById)) {
            node->localPlacement = *mat;
        }
    }

    processAggregates(relAggregates);
    processContained(relContained, entById);
    processBoundaries(relBoundaries, entById);
    processTasks(relProperties, entById);

    std::cout << "[build] Tree construction finished.\n";
}

void SpatialTree::print() const {
    if (root) root->print(); else std::cout << "(Leerer Baum)\n";
}

void SpatialTree::processAggregates(const std::vector<const Entity*>& rels) {
    if (!root) return;

    for (const auto* e : rels) {
        auto relating = getRelatingId(*e);
        if (!relating) continue;

        // A) Site -> Buildings
        if (*relating == root->id) {
            for (const auto& childId : getRelatedIds(*e)) {
                if (auto u = take(buildingPool_, childId)) {
                    BuildingNode* raw = u.get();
                    raw->parent = root.get(); 
                    root->addBuilding(std::move(u));
                    buildingById_[childId] = raw;
                }
            }
            continue;
        }

        // B) Building -> Stories
        if (auto itB = buildingById_.find(*relating); itB != buildingById_.end()) {
            BuildingNode* b = itB->second;
            for (const auto& childId : getRelatedIds(*e)) {
                if (auto u = take(storyPool_, childId)) {
                    StoryNode* raw = u.get();
                    raw->parent = b; 
                    b->addStory(std::move(u));
                    storyById_[childId] = raw;
                }
            }
            continue;
        }

        // C) Storey -> Spaces
        if (auto itS = storyById_.find(*relating); itS != storyById_.end()) {
            StoryNode* s = itS->second;
            for (const auto& childId : getRelatedIds(*e)) {
                if (auto u = take(spacePool_, childId)) {
                    SpaceNode* raw = u.get();
                    raw->parent = s; 
                    s->addSpace(std::move(u));
                    spaceById_[childId] = raw;
                }
            }
            continue;
        }
    }
}


void SpatialTree::processContained(const std::vector<const Entity*>& rels,
                                   const std::unordered_map<std::string, const Entity*>& entById) {
    if (!root) return;

    auto getOrCreateElement = [&](const std::string& elemId) -> ElementNode* {
        auto it = elementById_.find(elemId);
        if (it != elementById_.end()) return it->second;
        auto itEnt = entById.find(elemId);
        if (itEnt == entById.end()) return nullptr;
        const Entity* elEnt = itEnt->second;
        
      auto up = std::make_unique<ElementNode>(const_cast<Entity*>(elEnt), elEnt->id, getIfcName(*elEnt), elEnt->type);
        
        if (!up->geometryEntityId.empty()) {
            std::cout << "[Element] Found geometry " << up->geometryEntityId << " for Element " << elEnt->id << "\n";
        }

        if (auto mat = parseLocalPlacement(*elEnt, entById)) {
            up->localPlacement = *mat;
        }

        ElementNode* raw = up.get();
        elementById_[elEnt->id] = raw;
        elementPool_.emplace(elEnt->id, std::move(up));
        return raw;
    };

    for (const auto* e : rels) {
        auto relating = getRelatingId(*e);
        if (!relating) continue;
        const std::string& parentId = *relating;

        if (auto itSp = spaceById_.find(parentId); itSp != spaceById_.end()) {
            SpaceNode* space = itSp->second;
            for (const auto& childId : getRelatedIds(*e)) {
                auto itEnt = entById.find(childId);
                if (itEnt == entById.end() || !isDesiredElementType(itEnt->second->type)) continue;
                if (ElementNode* elem = getOrCreateElement(childId)) {
                    elem->parent = space;
                    space->addElement(elem);
                }
            }
            continue;
        }

        if (auto itSt = storyById_.find(parentId); itSt != storyById_.end()) {
            StoryNode* story = itSt->second;
            for (const auto& childId : getRelatedIds(*e)) {
                auto itEnt = entById.find(childId);
                if (itEnt == entById.end() || !isDesiredElementType(itEnt->second->type)) continue;
                if (ElementNode* elem = getOrCreateElement(childId)) {
                    elem->parent = story;
                    story->addElement(elem);
                }
            }
            continue;
        }
    }
}



void SpatialTree::processBoundaries(const std::vector<const Entity*>& rels,
                                    const std::unordered_map<std::string, const Entity*>& entById) {
    if (!root) return;

    auto getOrCreateElement = [&](const std::string& elemId) -> ElementNode* {
        auto it = elementById_.find(elemId);
        if (it != elementById_.end()) return it->second;
        auto itEnt = entById.find(elemId);
        if (itEnt == entById.end()) return nullptr;
        const Entity* elEnt = itEnt->second;
        
        auto up = std::make_unique<ElementNode>(const_cast<Entity*>(elEnt), elEnt->id, getIfcName(*elEnt), elEnt->type);
        if (auto mat = parseLocalPlacement(*elEnt, entById)) {
            up->localPlacement = *mat;
        }
        ElementNode* raw = up.get();
        elementById_[elEnt->id] = raw;
        elementPool_.emplace(elEnt->id, std::move(up));
        return raw;
    };

    for (const auto* e : rels) {
        // find rooms (Spaces)
        if (e->params.size() <= 4 || !e->params[4]) continue;

        auto relatingOpt = getRelatingId(*e);
        if (!relatingOpt) continue;
        const std::string& spaceId = *relatingOpt;

        auto itSp = spaceById_.find(spaceId);
        if (itSp == spaceById_.end()) continue;
        SpaceNode* space = itSp->second;

        auto boundary = std::make_unique<SpaceBoundaryNode>(const_cast<Entity*>(e), e->id, getIfcName(*e));
        boundary->parent = space;

        boundary->boundaryPoints = parsePolylinePoints(*e, entById); 
        
        std::optional<Matrix4x4> T_Boundary_Plane = parseBoundaryPlanePlacement(*e, entById);
        if (T_Boundary_Plane && !boundary->boundaryPoints.empty()) {

            
            // transformating points to global placement
            for (auto& point : boundary->boundaryPoints) {
                point = T_Boundary_Plane->transformPoint(point);
            }
        }
        

        //  RelatedElement connection via Attribut 8 (Index 7) ---
        if (e->params.size() > 5 && e->params[5]) {
            const ParamNode* elementRef = e->params[5].get();
            if (auto* elem = getOrCreateElement(elementRef->value)) {
                boundary->relatedElement = elem;
                elem->addBoundary(boundary.get()); 
            } else {
                 std::cerr << "WARNING: Could not find or create ElementNode for ID " 
                           << elementRef->value << " related to boundary " << boundary->id << ".\n";
            }
        } else {
            std::cerr << "WARNING: IfcRelSpaceBoundary " << e->id 
                      << " is missing RelatedElement attribute (Index 7) in IFC data.\n";
        }

        space->addBoundary(std::move(boundary));
    }
}

void SpatialTree::processTasks(const std::vector<const Entity*>& rels,
                               const std::unordered_map<std::string, const Entity*>& entById) {
    if (!root) return;

    for (const auto* rel : rels) {
        // Check if this is a RobotTask relation by Name (Attribute 2)
        std::string relName;
        if (rel->params.size() > 2 && rel->params[2]) {
            relName = rel->params[2]->value;
        }
        if (relName != "RobotTaskRelation") continue;

        // IFCRELDEFINESBYPROPERTIES: Attribute 5 = RelatingPropertyDefinition
        if (rel->params.size() <= 5 || !rel->params[5]) continue;
        
        const std::string& propSetId = rel->params[5]->value;
        auto itPropSet = entById.find(propSetId);
        if (itPropSet == entById.end() || itPropSet->second->type != "IFCPROPERTYSET") continue;
        
        const Entity* propSet = itPropSet->second;
        // Check if this is a RobotTask PropertySet by name
        std::string propSetName;
        if (propSet->params.size() > 2 && propSet->params[2]) {
            propSetName = propSet->params[2]->value;
        }
        if (propSetName.find("RobotTask") == std::string::npos) continue;

        // Parse properties: RobotStartPoint and RobotEndPoint
        std::string startSpaceId, endSpaceId;
        if (propSet->params.size() > 4 && propSet->params[4]) {
            const auto& propsNode = propSet->params[4];
            if (!propsNode->children.empty()) {
                for (const auto& propId_ptr : propsNode->children) {
                    if (!propId_ptr) continue;
                    const std::string& propId = propId_ptr->value;
                    auto itProp = entById.find(propId);
                    if (itProp == entById.end()) continue;
                    const Entity* prop = itProp->second;
                    
                    if (prop->type != "IFCPROPERTYSINGLEVALUE" || prop->params.size() < 3) continue;
                    
                    std::string propName = prop->params[0] ? prop->params[0]->value : "";
                    std::string propValue = prop->params[2] ? prop->params[2]->value : "";
                    
                    if (propName == "RobotStartPoint") startSpaceId = propValue;
                    else if (propName == "RobotEndPoint") endSpaceId = propValue;
                }
            }
        }

        if (startSpaceId.empty() || endSpaceId.empty()) continue;

        // Attribute 4 = RelatedObjects (the spaces this property is assigned to)
        if (rel->params.size() <= 4 || !rel->params[4]) continue;
        
        if (!rel->params[4]->children.empty()) {
            for (const auto& spaceId_ptr : rel->params[4]->children) {
                if (!spaceId_ptr) continue;
                const std::string& spaceId = spaceId_ptr->value;
                auto itSpace = spaceById_.find(spaceId);
                if (itSpace == spaceById_.end()) continue;
                
                SpaceNode* space = itSpace->second;
                RobotTask task;
                task.startSpaceId = startSpaceId;
                task.endSpaceId = endSpaceId;
                task.taskName = propSetName;
                space->robotTasks.push_back(task);
                
                std::cout << "[Task] Added RobotTask '" << propSetName << "' to Space " 
                          << space->id << " (" << startSpaceId << " -> " << endSpaceId << ")\n";
            }
        }
    }
}
