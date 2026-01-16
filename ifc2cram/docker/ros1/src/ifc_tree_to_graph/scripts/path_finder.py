from typing import List, Optional
import networkx as nx
import rospy
from graph_builder import _safe_get

def get_space_id_by_name(G: nx.Graph, space_name: str) -> Optional[str]:
    lower_name = space_name.lower()
    found_id = None
    for node_id, data in G.nodes(data=True):
        if data.get("type") == "space":
            node_name = data.get("data", {}).get("space_name", "")
            if node_name.lower() == lower_name:
                if found_id is not None:
                    rospy.logwarn(f"Mehrere Räume mit dem Namen '{space_name}' gefunden! Wähle den ersten: {found_id}")
                    return found_id
                found_id = node_id
    if found_id:
        return found_id
    else:
        rospy.logerr(f"Raumname '{space_name}' konnte keinem space_id im Graphen zugeordnet werden.")
        return None

def find_shortest_topological_path(G: nx.Graph, start_name: str, target_name: str) -> Optional[List[str]]:
    start_space_id = get_space_id_by_name(G, start_name)
    target_space_id = get_space_id_by_name(G, target_name)
    if not start_space_id or not target_space_id:
        return None
    rospy.loginfo(f"Suche Pfad von '{start_name}' ({start_space_id}) nach '{target_name}' ({target_space_id}).")
    try:
        path = nx.shortest_path(G, source=start_space_id, target=target_space_id, weight='weight')
        return path
    except nx.NetworkXNoPath:
        rospy.logwarn(f"Kein topologischer Pfad gefunden zwischen {start_name} und {target_name}.")
        return None
    except Exception as e:
        rospy.logerr(f"Fehler bei der Pfadberechnung: {e}")
        return None
