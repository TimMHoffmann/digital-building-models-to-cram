cmake_minimum_required(VERSION 3.0.2)
project(ifc_tree_to_graph)

find_package(catkin REQUIRED COMPONENTS
  rospy
  std_msgs
  geometry_msgs
  message_generation
)

add_service_files(
  FILES
  GetTaskPath.srv
)

generate_messages(
  DEPENDENCIES
  std_msgs
  geometry_msgs
)

catkin_package(
  CATKIN_DEPENDS rospy std_msgs geometry_msgs message_runtime
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

install(PROGRAMS
  scripts/ifc_tree_to_graph_node.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY srv/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/srv
  FILES_MATCHING PATTERN "*.srv"
)

install(DIRECTORY lisp/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/lisp
  FILES_MATCHING PATTERN "*.lisp"
)

install(FILES cram-ifc-building.asd
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)