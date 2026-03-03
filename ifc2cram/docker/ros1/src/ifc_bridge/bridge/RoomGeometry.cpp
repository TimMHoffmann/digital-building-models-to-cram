#include "RoomGeometry.hpp"
#include "model/SpaceNode.hpp"
#include "model/SpaceBoundaryNode.hpp"
#include "model/ElementNode.hpp"
#include "model/SpatialTree.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <iostream>

static bool ci_contains(const std::string &hay, const std::string &needle) {
    if (needle.empty()) return false;
    auto it = std::search(
        hay.begin(), hay.end(),
        needle.begin(), needle.end(),
        [](char a, char b){ return std::tolower(a) == std::tolower(b); }
    );
    return (it != hay.end());
}

std::unordered_map<std::string, RoomGeometry> buildAllRoomGeometries(const SpatialTree& tree) {
    std::unordered_map<std::string, RoomGeometry> out;
    if (!tree.root) return out;

    for (const auto& building : tree.root->buildings) {
        for (const auto& story : building->stories) {
            std::string story_id = story->id.empty() ? story->name : story->id;
            for (const auto& space : story->spaces) {
                RoomGeometry rg;
                rg.space_id = space->id;
                rg.space_name = space->name;
                rg.story_id = story_id;
                rg.story_name = story->name;

                for (const auto& bptr : space->boundaries) {
                    const SpaceBoundaryNode* b = bptr.get();
                    if (!b) continue;
                    BoundaryGeo bg;
                    bg.id = b->id;
                    bg.element_id = b->relatedElement ? b->relatedElement->id : std::string();
                    bg.element_type = b->relatedElement ? b->relatedElement->getIfcType() : std::string();
                    bg.is_virtual = false;

                    if (!b->relatedElement) {
                        std::cerr << "RoomGeometry: boundary " << bg.id << " has NO relatedElement\n";
                    } else {
                        if (!bg.is_virtual && ci_contains(bg.element_type, "IFCVIRTUALELEMENT")) bg.is_virtual = true;
                    }

                    Matrix4x4 abs = b->getAbsolutePlacement();
                    for (const auto& lp : b->boundaryPoints) {
                        std::array<double,3> wp = abs.transformPoint(lp);
                        bg.points.push_back(wp);
                    }
                    
                    if (!bg.element_type.empty() && ci_contains(bg.element_type, "IFCDOOR")) {
                        rg.doors.push_back(bg);
                    } else if (!bg.element_type.empty() && ci_contains(bg.element_type, "IFCCOLUMN")) {
                        rg.columns.push_back(bg);
                    } else {
                        rg.boundaries.push_back(bg);
                    }
                } // boundaries

                // Copy robot tasks from SpaceNode
                if (!space->robotTasks.empty()) {
                    rg.robotTasks = std::make_shared<std::vector<RobotTask>>(space->robotTasks);
                }

                out[rg.space_id] = std::move(rg);
            } // spaces
        } // stories
    } // buildings
    return out;
}

void buildRoomConnections(std::unordered_map<std::string, RoomGeometry>& rooms) {
    // map element_id -> list of space ids that reference it
    std::unordered_map<std::string, std::vector<std::string>> elem_to_spaces;
    for (const auto& kv : rooms) {
        const std::string& sid = kv.first;
        const RoomGeometry& rg = kv.second;
        // consider all boundaries (incl doors/columns)
        auto collect = [&](const BoundaryGeo& bg){
            if (!bg.element_id.empty()) elem_to_spaces[bg.element_id].push_back(sid);
        };
        for (const auto& b : rg.boundaries) collect(b);
        for (const auto& d : rg.doors) collect(d);
        for (const auto& c : rg.columns) collect(c);
    }

    // for each element_id with >1 spaces -> connect all pairs
    for (const auto& kv : elem_to_spaces) {
        const std::string& elem = kv.first;
        const auto& space_list = kv.second;
        if (space_list.size() < 2) continue;

        bool is_door = false;
        bool is_virtual = false;
        
        // try to find element_type by scanning rooms
        for (const auto& sid : space_list) {
            const auto& rg = rooms.at(sid);
            
            // Lambda to check a single boundary
            auto check_boundary = [&](const BoundaryGeo& bg) {
                if (bg.element_id == elem) {
                    if (ci_contains(bg.element_type, "IFCDOOR")) is_door = true;
                    if (ci_contains(bg.element_type, "IFCVIRTUALELEMENT")) is_virtual = true;
                }
            };
            
            // Check all boundary lists for this element
            for (const auto& b : rg.boundaries) check_boundary(b);
            for (const auto& d : rg.doors) check_boundary(d);
            for (const auto& c : rg.columns) check_boundary(c);
            
            if (is_door || is_virtual) break; 
        }

        if (!is_door && !is_virtual) {
            continue;
        }


        // 3. connect all pairs (simple undirected complete subgraph)
        for (size_t i = 0; i < space_list.size(); ++i) {
            for (size_t j = i+1; j < space_list.size(); ++j) {
                RoomConnection conn;
                conn.to_space_id = space_list[j];
                conn.via_element_id = elem;
                conn.via_door = is_door;
                conn.via_virtual = is_virtual;
                rooms[space_list[i]].connections.push_back(conn);

                RoomConnection conn_rev;
                conn_rev.to_space_id = space_list[i];
                conn_rev.via_element_id = elem;
                conn_rev.via_door = is_door;
                conn_rev.via_virtual = is_virtual;
                rooms[space_list[j]].connections.push_back(conn_rev);
            }
        }
    }
}