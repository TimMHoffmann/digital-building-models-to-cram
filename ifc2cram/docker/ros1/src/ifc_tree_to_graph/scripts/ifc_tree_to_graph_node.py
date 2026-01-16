#!/usr/bin/env python3
import rospy
import json
import networkx as nx
from std_msgs.msg import String
from config import TOPIC, GPICKLE_OUT
from graph_builder import build_graph_from_rooms
from building_urdf_generator import generate_building_urdf_from_graph
import os

def callback(msg: String):
    rospy.loginfo("Received rooms JSON, building graph...")
    try:
        data = json.loads(msg.data)
    except Exception as e:
        rospy.logerr(f"Failed to parse JSON: {e}")
        return
    print(data)
    G = build_graph_from_rooms(data)
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

def main():
    rospy.init_node("ifc_tree_to_graph_node", anonymous=True)
    rospy.Subscriber(TOPIC, String, callback)
    rospy.loginfo(f"ifc_tree_to_graph_node subscribing to {TOPIC}")
    rospy.spin()

if __name__ == "__main__":
    main()
