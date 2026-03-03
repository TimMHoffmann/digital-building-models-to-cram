import json
import networkx as nx
from typing import Dict, Any, List, Optional

def _safe_get(d: Dict[str, Any], key: str, default=None):
    return d.get(key, default) if isinstance(d, dict) else default

def build_graph_from_rooms(json_obj: Dict[str, Any]) -> nx.Graph:
    G = nx.Graph()
    rooms = json_obj.get("rooms", [])

    for room in rooms:
        sid = _safe_get(room, "space_id", "")
        if not sid:
            continue
        node_attrs = {
            "type": "space",
            "data": {
                "space_id": sid,
                "space_name": _safe_get(room, "space_name", ""),
                "story_id": _safe_get(room, "story_id", ""),
                "story_name": _safe_get(room, "story_name", ""),
                "boundaries": _safe_get(room, "boundaries", []),
                "columns": _safe_get(room, "columns", []),
                "doors": _safe_get(room, "doors", []),
                "connections": _safe_get(room, "connections", []),
                "robot_tasks": _safe_get(room, "robot_tasks", []),
                "raw": room
                
            }
        }
        G.add_node(sid, **node_attrs)

    for room in rooms:
        sid = _safe_get(room, "space_id", "")
        if not sid or sid not in G:
            continue

        for d in _safe_get(room, "doors", []):
            elem_id = _safe_get(d, "element_id", "")
            if not elem_id:
                continue
            enode = elem_id
            data = {
                "boundary_id": _safe_get(d, "id", ""),
                "element_id": elem_id,
                "element_type": "IFCDOOR",
                "points": _safe_get(d, "points", [])
            }
            if not G.has_node(enode):
                G.add_node(enode, type="door", data=data, spaces=[sid])
            else:
                if sid not in G.nodes[enode].setdefault("spaces", []):
                    G.nodes[enode]["spaces"].append(sid)
            if not G.has_edge(sid, enode):
                G.add_edge(sid, enode, relation="HAS_DOOR")

    for room in rooms:
        sid = _safe_get(room, "space_id", "")
        for conn in _safe_get(room, "connections", []):
            to_sid = _safe_get(conn, "to_space_id", "")
            via_elem = _safe_get(conn, "via_element_id", "")
            via_door = bool(_safe_get(conn, "via_door", False))
            via_virtual = bool(_safe_get(conn, "via_virtual", False))
            if not sid or not to_sid:
                continue
            if via_elem:
                enode = via_elem
                if not G.has_node(enode):
                    G.add_node(enode, type="element", data={"element_id": via_elem, "points": []}, spaces=[])
                if sid not in G.nodes[enode].setdefault("spaces", []):
                    G.nodes[enode]["spaces"].append(sid)
                if to_sid not in G.nodes[enode]["spaces"]:
                    G.nodes[enode]["spaces"].append(to_sid)
                if not G.has_edge(sid, enode):
                    G.add_edge(sid, enode, relation="CONNECTED_VIA_ELEMENT", element_id=via_elem, via_door=via_door, via_virtual=via_virtual)
                if not G.has_edge(to_sid, enode):
                    G.add_edge(to_sid, enode, relation="CONNECTED_VIA_ELEMENT", element_id=via_elem, via_door=via_door, via_virtual=via_virtual)
            else:
                if not G.has_edge(sid, to_sid):
                    G.add_edge(sid, to_sid, relation="CONNECTED", element_id="", via_door=via_door, via_virtual=via_virtual)

    for u, v, data in G.edges(data=True):
        if data.get('relation') in ['CONNECTED_VIA_ELEMENT', 'HAS_DOOR']:
            data['weight'] = 1
        else:
            data['weight'] = 999

    return G
