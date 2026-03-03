cmake_minimum_required(VERSION 3.0.2)
project(ifc_bridge)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(catkin REQUIRED COMPONENTS 
  roscpp    
  geometry_msgs
)

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR} 
)

add_executable(ifc_parser_node
    parser.tab.cpp
    lex.yy.cpp
    bridge/toifcgraph.cpp
    bridge/RoomGeometry.cpp
    utils/util.cpp
    model/Entity.cpp
    model/Matrix4x4.cpp
    model/ParamNode.cpp
    model/Relations.cpp
    model/SpatialNode.cpp
    model/SpatialTree.cpp
    model/SiteNode.cpp
    model/BuildingNode.cpp
    model/StoryNode.cpp
    model/SpaceNode.cpp
    model/ElementNode.cpp
    model/SpaceBoundaryNode.cpp
    model/RobotTask.cpp
)

target_link_libraries(ifc_parser_node
    ${catkin_LIBRARIES}     
    -lfl 
    -lm
)

catkin_package(
  CATKIN_DEPENDS roscpp geometry_msgs
)

install(TARGETS ifc_parser_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)