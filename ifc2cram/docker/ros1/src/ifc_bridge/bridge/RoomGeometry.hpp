#pragma once
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include "model/RobotTask.hpp"

struct BoundaryGeo {
    std::string id;
    std::string element_id;
    std::string element_type;
    bool is_virtual = false;
    std::vector<std::array<double,3>> points;
};

struct RoomConnection {
    std::string to_space_id;
    std::string via_element_id; 
    bool via_door = false;
    bool via_virtual = false;
};

struct RoomGeometry {
    std::string space_id;
    std::string space_name;
    std::string story_id;
    std::string story_name;

    std::vector<BoundaryGeo> boundaries;
    std::vector<BoundaryGeo> doors;
    std::vector<BoundaryGeo> columns;

    std::vector<RoomConnection> connections;
    std::shared_ptr<std::vector<RobotTask>> robotTasks;

    size_t numBoundaries() const { return boundaries.size(); }
    size_t numDoors() const { return doors.size(); }
    size_t numColumns() const { return columns.size(); }
};

class SpatialTree;
class SpaceNode;

std::unordered_map<std::string, RoomGeometry> buildAllRoomGeometries(const SpatialTree& tree);

void buildRoomConnections(std::unordered_map<std::string, RoomGeometry>& rooms);