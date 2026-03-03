#!/usr/bin/env python3
import rospy
import json
import networkx as nx
from std_msgs.msg import String
from geometry_msgs.msg import Point
from config import TOPIC, GPICKLE_OUT
from graph_builder import build_graph_from_rooms
from building_urdf_generator import generate_building_urdf_from_graph
from path_finder import (
    find_shortest_topological_path_by_id,
    build_waypoint_path,
)
from ifc_tree_to_graph.srv import GetTaskPath, GetTaskPathResponse
import os

_GRAPH = None

def callback(msg: String):
    rospy.loginfo("Received rooms JSON, building graph...")
    try:
        data = json.loads(msg.data)
    except Exception as e:
        rospy.logerr(f"Failed to parse JSON: {e}")
        return
    print(data)
    G = build_graph_from_rooms(data)
    global _GRAPH
    _GRAPH = G
    rospy.loginfo(f"Graph built: nodes={G.number_of_nodes()} edges={G.number_of_edges()}")
   
    try:
        nx.write_gpickle(G, GPICKLE_OUT)
        rospy.loginfo(f"Graph saved to {GPICKLE_OUT}")
    except Exception as e:
        rospy.logwarn(f"Failed to write graph file: {e}")
    space_nodes = [n for n, d in G.nodes(data=True) if d.get("type") == "space"]
    rospy.loginfo(f"Spaces: {len(space_nodes)}")
    
    generate_urdf = rospy.get_param('~generate_building_urdf', True)
    if generate_urdf:
        output_dir = rospy.get_param('~urdf_output_dir', '/root/cram_ws/src/ifc_tree_to_graph/output')
        building_name = rospy.get_param('~building_name', 'building')
        rospy.loginfo(f"Generating building URDF to {output_dir}...")
        try:
            urdf_path = generate_building_urdf_from_graph(G, output_dir, building_name)
            rospy.loginfo(f"Building URDF generated: {urdf_path}")
        except Exception as e:
            rospy.logerr(f"Failed to generate building URDF: {e}")
            import traceback
            traceback.print_exc()

def _load_graph_from_disk() -> nx.Graph:
    try:
        if hasattr(nx, "read_gpickle"):
            return nx.read_gpickle(GPICKLE_OUT)
    except Exception:
        pass
    try:
        from networkx.readwrite import gpickle as nx_gpickle
        return nx_gpickle.read_gpickle(GPICKLE_OUT)
    except Exception:
        return None

def _get_graph() -> nx.Graph:
    global _GRAPH
    if _GRAPH is not None:
        return _GRAPH
    if os.path.exists(GPICKLE_OUT):
        _GRAPH = _load_graph_from_disk()
        return _GRAPH
    return None

def _find_robot_task(G: nx.Graph, task_name: str):
    for node_id, data in G.nodes(data=True):
        if data.get("type") != "space":
            continue
        room_data = data.get("data", {})
        tasks = room_data.get("robot_tasks", [])
        for t in tasks:
            if t.get("task_name") == task_name:
                return t
    return None

def handle_get_task_path(req: GetTaskPath):
    G = _get_graph()
    if G is None:
        return GetTaskPathResponse(False, "Graph not available yet", req.task_name, "", "", [], [], [])

    task = _find_robot_task(G, req.task_name)
    if not task:
        return GetTaskPathResponse(False, "Task not found", req.task_name, "", "", [], [], [])

    start_id = task.get("start_space_id", "")
    end_id = task.get("end_space_id", "")
    if not start_id or not end_id:
        return GetTaskPathResponse(False, "Task missing start/end space id", req.task_name, start_id, end_id, [], [], [])

    path_nodes = find_shortest_topological_path_by_id(G, start_id, end_id)
    if not path_nodes:
        return GetTaskPathResponse(False, "No path found", req.task_name, start_id, end_id, [], [], [])

    # Hole alle Punkte aus den Lower-Slab-Boundaries
    try:
        waypoint_path = build_waypoint_path(G, path_nodes)
        rospy.loginfo(f"Generated {len(waypoint_path)} waypoints for path")
    except Exception as e:
        rospy.logerr(f"Error building waypoint path: {e}")
        import traceback
        traceback.print_exc()
        waypoint_path = []
    
    if not waypoint_path:
        rospy.logwarn("No waypoints generated")
        return GetTaskPathResponse(False, "No waypoints generated", req.task_name, start_id, end_id, path_nodes, [], [])
    
    space_points = []
    for point in waypoint_path:
        space_points.append(Point(x=point[0], y=point[1], z=point[2]))

    return GetTaskPathResponse(True, "OK", req.task_name, start_id, end_id, path_nodes, path_nodes, space_points)

def main():
    rospy.init_node("ifc_tree_to_graph_node", anonymous=True)
    rospy.Subscriber(TOPIC, String, callback)
    rospy.Service("ifc_tree_to_graph/get_task_path", GetTaskPath, handle_get_task_path)
    rospy.loginfo(f"ifc_tree_to_graph_node subscribing to {TOPIC}")
    rospy.spin()

if __name__ == "__main__":
    main()
