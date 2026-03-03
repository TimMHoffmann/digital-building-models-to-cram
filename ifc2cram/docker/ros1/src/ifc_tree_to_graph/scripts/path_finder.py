from typing import List, Optional, Tuple
import networkx as nx
import rospy


def get_space_id_by_name(G: nx.Graph, space_name: str) -> Optional[str]:
    """Resolve space name to space id (first match)."""
    if not space_name:
        return None
    lower_name = space_name.lower()
    found_id = None
    for node_id, data in G.nodes(data=True):
        if data.get("type") != "space":
            continue
        node_name = data.get("data", {}).get("space_name", "")
        if node_name.lower() == lower_name:
            if found_id is not None:
                rospy.logwarn(
                    f"Multiple spaces with name '{space_name}' found! Using first: {found_id}"
                )
                return found_id
            found_id = node_id

    if found_id:
        return found_id

    rospy.logerr(
        f"Space name '{space_name}' could not be resolved to any space_id in graph."
    )
    return None


def find_shortest_topological_path(
    G: nx.Graph, start_name: str, target_name: str
) -> Optional[List[str]]:
    """Find shortest path between two spaces by their names."""
    start_space_id = get_space_id_by_name(G, start_name)
    target_space_id = get_space_id_by_name(G, target_name)
    if not start_space_id or not target_space_id:
        return None
    return find_shortest_topological_path_by_id(G, start_space_id, target_space_id)


def find_shortest_topological_path_by_id(
    G: nx.Graph, start_space_id: str, target_space_id: str
) -> Optional[List[str]]:
    """Find shortest path between two spaces (including doors/virtual nodes)."""
    if not start_space_id or not target_space_id:
        return None
    if start_space_id not in G or target_space_id not in G:
        rospy.logwarn(
            f"Start/target not in graph: start='{start_space_id}', target='{target_space_id}'"
        )
        return None

    rospy.loginfo(
        f"Finding path from '{start_space_id}' to '{target_space_id}'."
    )
    try:
        path = nx.shortest_path(
            G, source=start_space_id, target=target_space_id, weight="weight"
        )
        print(path)
        return path if path else None
    except nx.NetworkXNoPath:
        rospy.logwarn(
            f"No topological path found between {start_space_id} and {target_space_id}."
        )
        return None
    except Exception as e:
        rospy.logerr(f"Error computing path: {e}")
        return None



def _get_door_center(
    G: nx.Graph, door_id: str
) -> Optional[Tuple[float, float, float]]:
    """Compute door center from boundary centers (4 boundaries)."""
    node = G.nodes.get(door_id, {})
    if node.get("type") != "door":
        return None
    data = node.get("data", {})
    door_boundaries = data.get("boundaries", [])
    if not door_boundaries:
        points = data.get("points", [])
        if not points:
            return None
        sx = sy = 0.0
        min_z = None
        for x, y, z in points:
            sx += x
            sy += y
            if min_z is None or z < min_z:
                min_z = z
        count = float(len(points))
        center_xy = (sx / count, sy / count)
        if min_z is None:
            return None
        return (center_xy[0], center_xy[1], min_z)

    all_centers = []
    min_z = None
    for boundary in door_boundaries:
        b_points = boundary.get("points", [])
        if b_points:
            sx = sy = 0.0
            for x, y, z in b_points:
                sx += x
                sy += y
                if min_z is None or z < min_z:
                    min_z = z
            count = float(len(b_points))
            all_centers.append((sx / count, sy / count))

    if not all_centers:
        return None

    sx = sy = 0.0
    for x, y in all_centers:
        sx += x
        sy += y
    count = float(len(all_centers))
    center_xy = (sx / count, sy / count)
    if min_z is None:
        return None
    return (center_xy[0], center_xy[1], min_z)


def _get_slab_center(
    G: nx.Graph, space_id: str
) -> Optional[Tuple[float, float, float]]:
    """Compute center of floor slab for a space."""
    node = G.nodes.get(space_id, {})
    if node.get("type") != "space":
        return None
    data = node.get("data", {})
    boundaries = data.get("boundaries", [])

    slab_boundaries = [
        b for b in boundaries
        if b.get("element_type", "").upper() in ["IFCSLAB"]
    ]

    if not slab_boundaries:
        return None

    all_centers = []
    min_z = None
    for boundary in slab_boundaries:
        b_points = boundary.get("points", [])
        if b_points:
            sx = sy = 0.0
            for x, y, z in b_points:
                sx += x
                sy += y
                if min_z is None or z < min_z:
                    min_z = z
            count = float(len(b_points))
            all_centers.append((sx / count, sy / count))

    if not all_centers:
        return None

    sx = sy = 0.0
    for x, y in all_centers:
        sx += x
        sy += y
    count = float(len(all_centers))
    center_xy = (sx / count, sy / count)
    if min_z is None:
        return None
    return (center_xy[0], center_xy[1], min_z)


def _get_points_center_min_z(
    G: nx.Graph, node_id: str
) -> Optional[Tuple[float, float, float]]:
    """Compute center from node points using min Z."""
    node = G.nodes.get(node_id, {})
    data = node.get("data", {})
    points = data.get("points", [])
    if not points:
        return None
    sx = sy = 0.0
    min_z = None
    for x, y, z in points:
        sx += x
        sy += y
        if min_z is None or z < min_z:
            min_z = z
    count = float(len(points))
    center_xy = (sx / count, sy / count)
    if min_z is None:
        return None
    return (center_xy[0], center_xy[1], min_z)


def build_waypoint_path(G: nx.Graph, space_path: List[str]) -> List[Tuple[float, float, float]]:
    """
    Build waypoint list from room slabs and door centers.
    """
    if not space_path:
        return []

    waypoints: List[Tuple[float, float, float]] = []

    for idx, node_id in enumerate(space_path):
        node_type = G.nodes.get(node_id, {}).get("type")
        if node_type == "space":
            slab_center = _get_slab_center(G, node_id)
            if slab_center:
                waypoints.append(slab_center)
            else:
                rospy.logwarn(f"No slab center found for space {node_id}")
            continue

        if node_type == "door":
            door_center = _get_door_center(G, node_id)
            if door_center:
                waypoints.append(door_center)
            else:
                fallback_center = _get_points_center_min_z(G, node_id)
                if fallback_center:
                    waypoints.append(fallback_center)
                else:
                    rospy.logdebug(f"No door center or points for {node_id}")
        else:
            fallback_center = _get_points_center_min_z(G, node_id)
            if fallback_center:
                waypoints.append(fallback_center)
            else:
                rospy.logdebug(
                    f"Skipping node {node_id} (type={node_type}) - no points"
                )

    return waypoints

