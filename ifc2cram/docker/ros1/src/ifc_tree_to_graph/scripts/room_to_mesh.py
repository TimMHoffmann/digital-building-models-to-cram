#!/usr/bin/env python3
import json
import numpy as np
from shapely.geometry import Polygon, MultiPolygon
from shapely.ops import unary_union
import trimesh
from typing import Dict, List, Tuple, Optional
import os
from pathlib import Path
import networkx as nx


class RoomMeshGenerator:

    def __init__(self):
        self.tolerance = 0.01  # Tolerance for point comparison
    
    def _is_door_in_boundary(self, door_points, boundary_points, tolerance=0.05):

        if len(door_points) < 3 or len(boundary_points) < 3:
            return False
        
        door_pts = np.array(door_points)
        boundary_pts = np.array(boundary_points)
        
        # Get bounding boxes
        door_min = door_pts.min(axis=0)
        door_max = door_pts.max(axis=0)
        door_center = door_pts.mean(axis=0)
        
        boundary_min = boundary_pts.min(axis=0)
        boundary_max = boundary_pts.max(axis=0)
        
        # Check if door center is within boundary bounds (with tolerance)
        within_x = (boundary_min[0] - tolerance) <= door_center[0] <= (boundary_max[0] + tolerance)
        within_y = (boundary_min[1] - tolerance) <= door_center[1] <= (boundary_max[1] + tolerance)
        within_z = (boundary_min[2] - tolerance) <= door_center[2] <= (boundary_max[2] + tolerance)
        
        # Also check if they share the same plane (for planar walls)
        # A wall is planar if one coordinate has very low variance
        boundary_std = boundary_pts.std(axis=0)
        
        shares_plane = False
        if boundary_std[0] < tolerance:  # Wall in YZ plane (constant X)
            if abs(door_center[0] - boundary_pts[:, 0].mean()) < tolerance:
                shares_plane = True
        elif boundary_std[1] < tolerance:  # Wall in XZ plane (constant Y)
            if abs(door_center[1] - boundary_pts[:, 1].mean()) < tolerance:
                shares_plane = True
        elif boundary_std[2] < tolerance:  # Wall in XY plane (constant Z)
            if abs(door_center[2] - boundary_pts[:, 2].mean()) < tolerance:
                shares_plane = True
        
        return (within_x and within_y and within_z) and shares_plane
    
    def _cut_door_from_wall(self, wall_points, door_points, tolerance=0.01):
        """
        Cut a door opening from a wall by splitting it into 4 rectangles around the door.
        Returns a list of rectangular wall segments (top, bottom, left, right of door).
        """
        if len(wall_points) < 4 or len(door_points) < 3:
            return None
        
        wall_pts = np.array(wall_points)
        door_pts = np.array(door_points)
        
        # Get wall and door bounding boxes
        wall_min = wall_pts.min(axis=0)
        wall_max = wall_pts.max(axis=0)
        door_min = door_pts.min(axis=0)
        door_max = door_pts.max(axis=0)
        
        # Determine the plane of the wall (which axis is constant)
        wall_std = wall_pts.std(axis=0)
        
        rectangles = []
        
        if wall_std[0] < tolerance:  # YZ plane (X constant at wall_x)
            wall_x = wall_pts[:, 0].mean()
            
            # Bottom rectangle (below door)
            if door_min[2] > wall_min[2] + tolerance:
                rect = np.array([
                    [wall_x, wall_min[1], wall_min[2]],
                    [wall_x, wall_max[1], wall_min[2]],
                    [wall_x, wall_max[1], door_min[2]],
                    [wall_x, wall_min[1], door_min[2]]
                ])
                rectangles.append(rect)
            
            # Top rectangle (above door)
            if door_max[2] < wall_max[2] - tolerance:
                rect = np.array([
                    [wall_x, wall_min[1], door_max[2]],
                    [wall_x, wall_max[1], door_max[2]],
                    [wall_x, wall_max[1], wall_max[2]],
                    [wall_x, wall_min[1], wall_max[2]]
                ])
                rectangles.append(rect)
            
            # Left rectangle (left of door)
            if door_min[1] > wall_min[1] + tolerance:
                rect = np.array([
                    [wall_x, wall_min[1], door_min[2]],
                    [wall_x, door_min[1], door_min[2]],
                    [wall_x, door_min[1], door_max[2]],
                    [wall_x, wall_min[1], door_max[2]]
                ])
                rectangles.append(rect)
            
            # Right rectangle (right of door)
            if door_max[1] < wall_max[1] - tolerance:
                rect = np.array([
                    [wall_x, door_max[1], door_min[2]],
                    [wall_x, wall_max[1], door_min[2]],
                    [wall_x, wall_max[1], door_max[2]],
                    [wall_x, door_max[1], door_max[2]]
                ])
                rectangles.append(rect)
                
        elif wall_std[1] < tolerance:  # XZ plane (Y constant at wall_y)
            wall_y = wall_pts[:, 1].mean()
            
            # Bottom rectangle (below door)
            if door_min[2] > wall_min[2] + tolerance:
                rect = np.array([
                    [wall_min[0], wall_y, wall_min[2]],
                    [wall_max[0], wall_y, wall_min[2]],
                    [wall_max[0], wall_y, door_min[2]],
                    [wall_min[0], wall_y, door_min[2]]
                ])
                rectangles.append(rect)
            
            # Top rectangle (above door)
            if door_max[2] < wall_max[2] - tolerance:
                rect = np.array([
                    [wall_min[0], wall_y, door_max[2]],
                    [wall_max[0], wall_y, door_max[2]],
                    [wall_max[0], wall_y, wall_max[2]],
                    [wall_min[0], wall_y, wall_max[2]]
                ])
                rectangles.append(rect)
            
            # Left rectangle (left of door)
            if door_min[0] > wall_min[0] + tolerance:
                rect = np.array([
                    [wall_min[0], wall_y, door_min[2]],
                    [door_min[0], wall_y, door_min[2]],
                    [door_min[0], wall_y, door_max[2]],
                    [wall_min[0], wall_y, door_max[2]]
                ])
                rectangles.append(rect)
            
            # Right rectangle (right of door)
            if door_max[0] < wall_max[0] - tolerance:
                rect = np.array([
                    [door_max[0], wall_y, door_min[2]],
                    [wall_max[0], wall_y, door_min[2]],
                    [wall_max[0], wall_y, door_max[2]],
                    [door_max[0], wall_y, door_max[2]]
                ])
                rectangles.append(rect)
                
        elif wall_std[2] < tolerance:  # XY plane (Z constant at wall_z)
            wall_z = wall_pts[:, 2].mean()
            
            # Bottom rectangle
            if door_min[1] > wall_min[1] + tolerance:
                rect = np.array([
                    [wall_min[0], wall_min[1], wall_z],
                    [wall_max[0], wall_min[1], wall_z],
                    [wall_max[0], door_min[1], wall_z],
                    [wall_min[0], door_min[1], wall_z]
                ])
                rectangles.append(rect)
            
            # Top rectangle
            if door_max[1] < wall_max[1] - tolerance:
                rect = np.array([
                    [wall_min[0], door_max[1], wall_z],
                    [wall_max[0], door_max[1], wall_z],
                    [wall_max[0], wall_max[1], wall_z],
                    [wall_min[0], wall_max[1], wall_z]
                ])
                rectangles.append(rect)
            
            # Left rectangle
            if door_min[0] > wall_min[0] + tolerance:
                rect = np.array([
                    [wall_min[0], door_min[1], wall_z],
                    [door_min[0], door_min[1], wall_z],
                    [door_min[0], door_max[1], wall_z],
                    [wall_min[0], door_max[1], wall_z]
                ])
                rectangles.append(rect)
            
            # Right rectangle
            if door_max[0] < wall_max[0] - tolerance:
                rect = np.array([
                    [door_max[0], door_min[1], wall_z],
                    [wall_max[0], door_min[1], wall_z],
                    [wall_max[0], door_max[1], wall_z],
                    [door_max[0], door_max[1], wall_z]
                ])
                rectangles.append(rect)
        else:
            # Non-planar wall
            return None
        
        return rectangles if len(rectangles) > 0 else None
    

    def create_room_mesh(self, room_data: Dict) -> Tuple[Optional[trimesh.Trimesh], Optional[np.ndarray]]:
        """Create a 3D mesh for the room from its boundaries and doors"""
        boundaries = room_data.get("boundaries", [])
        doors = room_data.get("doors", [])
        
        if not boundaries:
            print("No boundaries found")
            return None, None
        
        # Collect all points to determine room bounds
        all_points = []
        for boundary in boundaries:
            points = boundary.get("points", [])
            if points:
                all_points.extend(points)
        
        # Also include door points for complete bounds
        for door in doors:
            points = door.get("points", [])
            if points:
                all_points.extend(points)
        
        if not all_points:
            print("No valid points found")
            return None, None
        
        all_points_array = np.array(all_points)
        min_z = all_points_array[:, 2].min()
        max_z = all_points_array[:, 2].max()
        room_height = max_z - min_z
        
        print(f"  Room bounds: floor_z={min_z:.2f}m, ceiling_z={max_z:.2f}m, height={room_height:.2f}m")
        
        # Now build mesh from all boundary faces
        all_vertices = []
        all_faces = []
        vertex_offset = 0
        
        # Process each boundary
        for boundary in boundaries:
            element_type = boundary.get("element_type", "")
            points = boundary.get("points", [])
            is_virtual = boundary.get("is_virtual", False)
            boundary_id = boundary.get("id", "")
            
            # Skip virtual elements - these are openings
            if is_virtual or element_type == "IFCVIRTUALELEMENT":
                continue
            
            # Check if this wall has a door in it and cut it out
            wall_rectangles = None
            if element_type == "IFCWALLSTANDARDCASE":
                for door in doors:
                    door_points = door.get("points", [])
                    if self._is_door_in_boundary(door_points, points):
                        print(f"    Wall boundary {boundary_id} contains door {door.get('element_id')} - cutting hole")
                        wall_rectangles = self._cut_door_from_wall(points, door_points)
                        if wall_rectangles is not None:
                            print(f"      Wall split into {len(wall_rectangles)} rectangles around door")
                        else:
                            print(f"      Warning: Could not cut door hole, using full wall")
                        break
            
            # If wall was split into rectangles, add each rectangle separately
            if wall_rectangles is not None:
                for rect_idx, rect_points in enumerate(wall_rectangles):
                    if len(rect_points) >= 3:
                        boundary_vertices = rect_points
                        all_vertices.extend(boundary_vertices)
                        
                        # Triangulate rectangle (2 triangles)
                        tri1 = [vertex_offset, vertex_offset + 1, vertex_offset + 2]
                        tri2 = [vertex_offset, vertex_offset + 2, vertex_offset + 3]
                        all_faces.extend([tri1, tri2])
                        vertex_offset += 4
                continue
            
            # Normal processing for non-cut boundaries
            if len(points) >= 3:
                boundary_vertices = np.array(points)
                
                # Check if it's a planar face (all points on same plane)
                if len(points) == 4 or len(points) == 5:
                    # Quadrilateral or closed quad (5 points where last = first)
                    n_verts = 4 if len(points) == 5 and np.allclose(points[0], points[4]) else len(points)
                    
                    # Add vertices
                    all_vertices.extend(boundary_vertices[:n_verts])
                    
                    # Triangulate quad: two triangles
                    if n_verts == 4:
                        tri1 = [vertex_offset, vertex_offset + 1, vertex_offset + 2]
                        tri2 = [vertex_offset, vertex_offset + 2, vertex_offset + 3]
                        all_faces.extend([tri1, tri2])
                        vertex_offset += 4
                    else:
                        # More complex polygon - use fan triangulation
                        for i in range(1, n_verts - 1):
                            tri = [vertex_offset, vertex_offset + i, vertex_offset + i + 1]
                            all_faces.append(tri)
                        vertex_offset += n_verts
                else:
                    # Polygon with more points - fan triangulation
                    all_vertices.extend(boundary_vertices)
                    n_verts = len(points)
                    
                    for i in range(1, n_verts - 1):
                        tri = [vertex_offset, vertex_offset + i, vertex_offset + i + 1]
                        all_faces.append(tri)
                    vertex_offset += n_verts
        
        # Create trimesh
        if len(all_vertices) > 0 and len(all_faces) > 0:
            vertices_array = np.array(all_vertices)
            
            # Calculate centroid BEFORE creating mesh
            centroid = vertices_array.mean(axis=0)
            
            # Center the mesh around origin (0,0,0)
            # This way, the mesh is positioned via URDF joint origins
            centered_vertices = vertices_array - centroid
            
            mesh = trimesh.Trimesh(
                vertices=centered_vertices,
                faces=np.array(all_faces),
                process=True 
            )
            
            print(f"  Mesh created: {len(mesh.vertices)} vertices, {len(mesh.faces)} faces")
            print(f"  Original centroid: ({centroid[0]:.2f}, {centroid[1]:.2f}, {centroid[2]:.2f})")
            print(f"  Mesh now centered at origin")
            
            # Check if mesh is watertight
            if not mesh.is_watertight:
                print(f"  Warning: Mesh is not watertight (may have holes at doors/openings)")
            
            return mesh, centroid
        
        print("  Failed to create mesh: no valid faces")
        return None, None
    
    
    def export_mesh(self, mesh: trimesh.Trimesh, output_path: str, format: str = "stl"):
        """Export mesh to file"""
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        mesh.export(str(output_path), file_type=format)
        print(f"Mesh exported to: {output_path}")
    
    def create_urdf(self, mesh_filename: str, space_name: str, output_path: str):
        """Create URDF file for the room mesh"""
        urdf_content = f"""<?xml version="1.0"?>
<robot name="room_{space_name}">
  <link name="room_{space_name}_link">
    <visual>
      <geometry>
        <mesh filename="package://$(find ifc_tree_to_graph)/meshes/{mesh_filename}" scale="1.0 1.0 1.0"/>
      </geometry>
      <material name="room_material">
        <color rgba="0.8 0.8 0.8 1.0"/>
      </material>
    </visual>
    <collision>
      <geometry>
        <mesh filename="package://$(find ifc_tree_to_graph)/meshes/{mesh_filename}" scale="1.0 1.0 1.0"/>
      </geometry>
    </collision>
    <inertial>
      <mass value="1.0"/>
      <inertia ixx="1.0" ixy="0.0" ixz="0.0" iyy="1.0" iyz="0.0" izz="1.0"/>
    </inertial>
  </link>
</robot>
"""
        
        output_path = Path(output_path)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
        with open(output_path, 'w') as f:
            f.write(urdf_content)
        
        print(f"URDF exported to: {output_path}")
    

