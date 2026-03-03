#include <ros/ros.h>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "parser.tab.hpp" 
#include "lexer/lexer.hpp"
#include "model/SpatialTree.hpp" 
#include "parser_export.hpp"
#include "model/SpatialNode.hpp"
#include "model/SiteNode.hpp"
#include "model/BuildingNode.hpp"
#include "model/StoryNode.hpp"
#include "model/SpaceNode.hpp"
#include "model/ElementNode.hpp"
#include "model/SpaceBoundaryNode.hpp"
#include "model/RobotTask.hpp"
#include <std_msgs/String.h>
#include "RoomGeometry.hpp"

static std::string escapeJson(const std::string &s) {
    std::string out; out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

static std::string serializeRoomsToJson(const std::unordered_map<std::string, RoomGeometry>& rooms) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{ \"rooms\": [";
    bool firstRoom = true;
    for (const auto& kv : rooms) {
        if (!firstRoom) ss << ",";
        firstRoom = false;
        const RoomGeometry &r = kv.second;

        ss << "{";
        ss << "\"space_id\":\"" << escapeJson(r.space_id) << "\",";
        ss << "\"space_name\":\"" << escapeJson(r.space_name) << "\",";
        ss << "\"story_id\":\"" << escapeJson(r.story_id) << "\",";
        ss << "\"story_name\":\"" << escapeJson(r.story_name) << "\",";

        // boundaries
        ss << "\"boundaries\": [";
        bool firstB = true;
        for (const auto &b : r.boundaries) {
            if (!firstB) ss << ",";
            firstB = false;
            ss << "{";
            ss << "\"id\":\"" << escapeJson(b.id) << "\",";
            ss << "\"element_id\":\"" << escapeJson(b.element_id) << "\",";
            ss << "\"element_type\":\"" << escapeJson(b.element_type) << "\",";
            ss << "\"is_virtual\":" << (b.is_virtual ? "true" : "false") << ",";
            ss << "\"points\": [";
            bool firstP = true;
            for (const auto &pt : b.points) {
                if (!firstP) ss << ",";
                firstP = false;
                ss << "[" << pt[0] << "," << pt[1] << "," << pt[2] << "]";
            }
            ss << "]";
            ss << "}";
        }
        ss << "],";

        // doors
        ss << "\"doors\": [";
        bool firstD = true;
        for (const auto &d : r.doors) {
            if (!firstD) ss << ",";
            firstD = false;
            ss << "{";
            ss << "\"id\":\"" << escapeJson(d.id) << "\",";
            ss << "\"element_id\":\"" << escapeJson(d.element_id) << "\",";
            ss << "\"points\": [";
            bool firstP = true;
            for (const auto &pt : d.points) {
                if (!firstP) ss << ",";
                firstP = false;
                ss << "[" << pt[0] << "," << pt[1] << "," << pt[2] << "]";
            }
            ss << "]";
            ss << "}";
        }
        ss << "],";

        // columns
        ss << "\"columns\": [";
        bool firstC = true;
        for (const auto &c : r.columns) {
            if (!firstC) ss << ",";
            firstC = false;
            ss << "{";
            ss << "\"id\":\"" << escapeJson(c.id) << "\",";
            ss << "\"element_id\":\"" << escapeJson(c.element_id) << "\",";
            ss << "\"points\": [";
            bool firstP = true;
            for (const auto &pt : c.points) {
                if (!firstP) ss << ",";
                firstP = false;
                ss << "[" << pt[0] << "," << pt[1] << "," << pt[2] << "]";
            }
            ss << "]";
            ss << "}";
        }
        ss << "],";

        // connections
        ss << "\"connections\": [";
        bool firstConn = true;
        for (const auto &cn : r.connections) {
            if (!firstConn) ss << ",";
            firstConn = false;
            ss << "{";
            ss << "\"to_space_id\":\"" << escapeJson(cn.to_space_id) << "\",";
            ss << "\"via_element_id\":\"" << escapeJson(cn.via_element_id) << "\",";
            ss << "\"via_door\":" << (cn.via_door ? "true" : "false") << ",";
            ss << "\"via_virtual\":" << (cn.via_virtual ? "true" : "false");
            ss << "}";
        }
        ss << "],";

        // robot tasks
        ss << "\"robot_tasks\": [";
        bool firstTask = true;
        if (r.robotTasks) {
            for (const auto &task : *r.robotTasks) {
                if (!firstTask) ss << ",";
                firstTask = false;
                ss << "{";
                ss << "\"task_name\":\"" << escapeJson(task.taskName) << "\",";
                ss << "\"start_space_id\":\"" << escapeJson(task.startSpaceId) << "\",";
                ss << "\"end_space_id\":\"" << escapeJson(task.endSpaceId) << "\"";
                ss << "}";
            }
        }
        ss << "]";

        ss << "}";
    }
    ss << "]}";
    return ss.str();
}

int main(int argc, char** argv) {
    ros::init(argc, argv, "ifc_parser_node");
    ros::NodeHandle nh;
    if (argc <= 1) {
        ROS_ERROR("IFC Parser: Please give the IFC filename as argument.");
        return 1;
    }
    std::string filename = argv[1];

    std::ifstream infile(filename);    
    if (!infile) {
        ROS_ERROR_STREAM("IFC Parser: Error opening file " << filename);
        return 1;
    }

    IfcLexer lexer(&infile);          
    yy::parser parser(lexer);

    ROS_INFO_STREAM("IFC Parser: Starting to parse file " << filename);

    int parse_result = parser.parse();
    if (parse_result == 0) {
        ROS_INFO("IFC Parser: Parsing successful.");
        
        const auto& parsed_entities_ref = getParsedEntities(); 
        
        SpatialTree tree;
        tree.build(parsed_entities_ref); 

        if(tree.root) {

                auto rooms = buildAllRoomGeometries(tree);
                buildRoomConnections(rooms);

                {
                    ros::Publisher rooms_pub = nh.advertise<std_msgs::String>("ifc_bridge/rooms_json", 1, true);
                    std_msgs::String msg;
                    msg.data = serializeRoomsToJson(rooms);
                    rooms_pub.publish(msg);
                    ROS_INFO_STREAM("IFC Parser: published rooms JSON on /ifc_bridge/rooms_json (size=" << msg.data.size() << ")");
                    ros::Duration(0.2).sleep();
                }

                for (const auto& kv : rooms) {
                    const std::string& sid = kv.first;
                    const RoomGeometry& rg = kv.second;
                    if (tree.root) {
                        for (const auto& building : tree.root->buildings) {
                            for (const auto& story : building->stories) {
                                for (const auto& space_ptr : story->spaces) {
                                    if (space_ptr->id == sid) {
                                        for (const auto& c : rg.connections) {
                                            space_ptr->addConnectedSpace(c.to_space_id);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                for (const auto& kv : rooms) {
                    const RoomGeometry& rg = kv.second;
                    std::cout << "Space " << rg.space_id << " Name: " << rg.space_name << " (story=" << rg.story_id << "story_name: " << rg.story_name << ") summary:"
                            << " boundaries=" << rg.numBoundaries()
                            << " doors=" << rg.numDoors()
                            << " columns=" << rg.numColumns()
                            << " connections=" << rg.connections.size()
                            << std::endl;
                    for (const auto& c : rg.connections) {
                        std::cout << "  connected to " << c.to_space_id
                                << " via element " << c.via_element_id
                                << (c.via_door ? " [door]" : "") << (c.via_virtual ? " [virtual]" : "") << std::endl;
                    }
                }
            
        } else {
            ROS_ERROR("IFC Parser: SpatialTree has no root node.");
        }



      
        
    } else {
        ROS_ERROR("IFC Parser: Parsing failed with error code %d.", parse_result);
    }
    
    ROS_INFO("IFC Parser: Node is shutting down.");
    return 0;
}