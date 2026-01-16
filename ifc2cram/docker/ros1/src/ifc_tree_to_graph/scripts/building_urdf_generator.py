#!/usr/bin/env python3
import networkx as nx
import numpy as np
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import xml.etree.ElementTree as ET
from xml.dom import minidom
from room_to_mesh import RoomMeshGenerator


class BuildingURDFGenerator:
    def __init__(self, package_name: str = "ifc_tree_to_graph"):
        self.package_name = package_name
        self.mesh_generator = RoomMeshGenerator()
        
    def generate_building_urdf(self, G: nx.Graph, output_dir: str, building_name: str = "building"):
     
        output_path = Path(output_dir)
        meshes_dir = output_path / "meshes"
        urdf_dir = output_path / "urdf"
        meshes_dir.mkdir(parents=True, exist_ok=True)
        urdf_dir.mkdir(parents=True, exist_ok=True)
        
        space_nodes = [(node_id, data) for node_id, data in G.nodes(data=True) 
                       if data.get("type") == "space"]
        
        if not space_nodes:
            print("No space nodes found in graph")
            return None
        
        print(f"Generating building URDF with {len(space_nodes)} rooms...")
        
        robot = ET.Element('robot', name=building_name)
        room_info = {}
        
        for space_id, node_data in space_nodes:
            room_data = node_data.get("data", {})
            space_name = room_data.get("space_name", "unknown")
            story_name = room_data.get("story_name", "unknown")
            
            print(f"Processing room: {space_name} (ID: {space_id})")
            
            # Generate mesh
            try:
                mesh, original_centroid = self.mesh_generator.create_room_mesh(room_data)
                
                if mesh is None or original_centroid is None:
                    print(f"  Warning: Could not create mesh for room {space_name}")
                    continue
                
                # Save room mesh
                clean_space_id = space_id.replace("#", "")
                mesh_filename = f"room_{space_name}_{clean_space_id}.stl"
                mesh_path = meshes_dir / mesh_filename
                
                self.mesh_generator.export_mesh(mesh, mesh_path, format="stl")
                
               
                center = original_centroid
                bounds = mesh.bounds  # [min_x, min_y, min_z], [max_x, max_y, max_z]
                
                room_info[space_id] = {
                    'name': space_name,
                    'clean_id': clean_space_id,
                    'mesh_filename': mesh_filename,
                    'center': center,
                    'bounds': bounds,
                    'story': story_name
                }
                
                # Create URDF link for this room
                self._add_room_link(robot, space_id, room_info[space_id])
                
                print(f"  ✓ Mesh created: {len(mesh.vertices)} vertices, {len(mesh.faces)} faces")
                
            except Exception as e:
                print(f"  Error processing room {space_name}: {e}")
                import traceback
                traceback.print_exc()
                continue
        
        # Create new robot element with proper order
        # We need to rebuild to ensure world is first
        robot_new = ET.Element('robot', name=building_name)
        
        # 1. Add base link first (world reference)
        self._add_base_link(robot_new)
        
        # 2. Copy all room links
        for link in robot.findall('link'):
            robot_new.append(link)
        
        # 3. Connect ALL rooms to world base
        for space_id, info in room_info.items():
            self._add_base_joint(robot_new, space_id, info)
        
        
        # Simply connect all rooms to world (already done in step 3 via _add_base_joint)
        # Room connectivity for navigation is handled by the graph structure, not URDF joints
        # If you need explicit connection joints, you must build a spanning tree first
        
        print(f"Skipping connection joints - using flat world->room structure to avoid URDF multi-parent errors")
        
        # Use the new robot structure
        robot = robot_new
        
        # Write URDF file
        urdf_filename = f"{building_name}.urdf"
        urdf_path = urdf_dir / urdf_filename
        
        # Pretty print XML
        xml_string = ET.tostring(robot, encoding='unicode')
        dom = minidom.parseString(xml_string)
        pretty_xml = dom.toprettyxml(indent="  ")
        
        # Remove extra blank lines
        pretty_xml = '\n'.join([line for line in pretty_xml.split('\n') if line.strip()])
        
        with open(urdf_path, 'w') as f:
            f.write(pretty_xml)
        
        print(f"\n{'='*60}")
        print(f"✓ Building URDF generated successfully!")
        print(f"  Rooms: {len(room_info)}")
        print(f"  URDF: {urdf_path}")
        print(f"  Meshes: {meshes_dir}/")
        print(f"\nTo visualize in RViz:")
        print(f"  roslaunch urdf_tutorial display.launch model:={urdf_path}")
        
        return urdf_path
    
    def _add_base_link(self, robot: ET.Element):
        """Add base world link"""
        link = ET.SubElement(robot, 'link', name='world')
        
        # Minimal visual (small sphere at origin)
        visual = ET.SubElement(link, 'visual')
        geometry = ET.SubElement(visual, 'geometry')
        sphere = ET.SubElement(geometry, 'sphere', radius="0.05")
        
        material = ET.SubElement(visual, 'material', name='world_material')
        color = ET.SubElement(material, 'color', rgba="1 0 0 1")
    
    def _add_room_link(self, robot: ET.Element, space_id: str, room_info: Dict):
        """Add a link for a room"""
        clean_id = room_info['clean_id']
        link_name = f"room_{room_info['name']}_{clean_id}"
        
        link = ET.SubElement(robot, 'link', name=link_name)
        
        # Visual (room mesh)
        visual = ET.SubElement(link, 'visual', name='room_visual')
        
        # Origin (center mesh at link origin)
        origin = ET.SubElement(visual, 'origin', 
                               xyz="0 0 0",
                               rpy="0 0 0")
        
        # Geometry - mesh
        geometry = ET.SubElement(visual, 'geometry')
        mesh_uri = f"package://{self.package_name}/output/meshes/{room_info['mesh_filename']}"
        mesh_elem = ET.SubElement(geometry, 'mesh', 
                                  filename=mesh_uri,
                                  scale="1 1 1")
        
        # Material (gray for room)
        material = ET.SubElement(visual, 'material', name=f"material_{clean_id}")
        color = ET.SubElement(material, 'color', rgba="0.8 0.8 0.8 0.7")
        
        # Add door opening meshes (in red color)
        door_mesh_files = room_info.get('door_mesh_files', [])
        for idx, door_filename in enumerate(door_mesh_files):
            door_visual = ET.SubElement(link, 'visual', name=f'door_visual_{idx}')
            door_origin = ET.SubElement(door_visual, 'origin', 
                                       xyz="0 0 0",
                                       rpy="0 0 0")
            door_geometry = ET.SubElement(door_visual, 'geometry')
            door_mesh_uri = f"package://{self.package_name}/output/meshes/{door_filename}"
            door_mesh_elem = ET.SubElement(door_geometry, 'mesh', 
                                          filename=door_mesh_uri,
                                          scale="1 1 1")
            door_material = ET.SubElement(door_visual, 'material', name=f"door_material_{clean_id}_{idx}")
            door_color = ET.SubElement(door_material, 'color', rgba="1.0 0.0 0.0 0.9")  # Red for doors
        
        # Collision (same as visual)
        collision = ET.SubElement(link, 'collision')
        origin_col = ET.SubElement(collision, 'origin', 
                                   xyz="0 0 0",
                                   rpy="0 0 0")
        geometry_col = ET.SubElement(collision, 'geometry')
        mesh_col = ET.SubElement(geometry_col, 'mesh', 
                                filename=mesh_uri,
                                scale="1 1 1")
        
        # Inertial (dummy values)
        inertial = ET.SubElement(link, 'inertial')
        mass = ET.SubElement(inertial, 'mass', value="1.0")
        inertia = ET.SubElement(inertial, 'inertia',
                               ixx="1.0", ixy="0.0", ixz="0.0",
                               iyy="1.0", iyz="0.0", izz="1.0")
    
    def _add_base_joint(self, robot: ET.Element, space_id: str, room_info: Dict):
        """Add joint connecting first room to world"""
        clean_id = room_info['clean_id']
        joint_name = f"world_to_room_{room_info['name']}_{clean_id}"
        child_link = f"room_{room_info['name']}_{clean_id}"
        
        joint = ET.SubElement(robot, 'joint', 
                             name=joint_name,
                             type='fixed')
        
        parent = ET.SubElement(joint, 'parent', link='world')
        child = ET.SubElement(joint, 'child', link=child_link)
        
        # Position at room center
        center = room_info['center']
        origin = ET.SubElement(joint, 'origin',
                              xyz=f"{center[0]} {center[1]} {center[2]}",
                              rpy="0 0 0")
    
    
    
def generate_building_urdf_from_graph(G: nx.Graph, output_dir: str, building_name: str = "building"):
    generator = BuildingURDFGenerator()
    generator.generate_building_urdf(G, output_dir, building_name)

