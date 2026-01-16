# IFC2Cram
Start the whole enviroment with docker-compose up --build
## Probleme
2. please activat for the container xhost +local: to get permission for screen forwarding



# IFC to Building URDF

## Data flow

```
IFC-File
    ↓
[ifc_parser_node]
    ↓
JSON (Rooms + Boundaries)
    ↓
[ifc_tree_to_graph_node]
    ↓
NetworkX-Graph (building_graph.gpickle)
    ↓
[building_urdf_generator]
    ↓
├─→ URDF (building.urdf)
└─→ Meshes (*.stl)
    ↓
[RViz]
    ↓
3D-Visualization
```

---

---

## Example-Workflow
## Show whole generating process to get urdf
1) Starting ros node to generate urdf
```bash
rosrun ifc_tree_to_graph ifc_tree_to_graph_node.py
```
2) Starting ros node to compile ifc
```bash 
/root/cram_ws/build/ifc_bridge/ifc_parser_node /root/ifc_files/Haus.ifc
``` 
3) Start rviz to look at the generated urdf
```bash
roslaunch ifc_tree_to_graph view_building.launch
```

##
## Show cram to urdf process
1) Ensure the catking_make is running
```bash
 catkin_make -DPython3_EXECUTABLE=/usr/bin/python3 \
                -DCMAKE_EXE_LINKER_FLAGS='-L/usr/local/lib/swipl/lib/x86_64-linux -Wl,-rpath,/usr/local/lib/swipl/lib/x86_64-linux' \
                -DCMAKE_INCLUDE_DIRS='/usr/local/include/swipl'
```

2) Launch bullet world
```bash
source devel/setup.bash && roslaunch ifc_tree_to_graph building_bullet_world.launch
```

3) Start emacs
```bash 
cd /root/cram_ws
source devel/setup.bash && roslisp_repl
```


4) Start bullet world with generated urdf
```bash 
(asdf:load-system :cram-bullet-reasoning)
(in-package :btr)
(roslisp:start-ros-node "bullet_world")
(setf *current-bullet-world* (make-instance 'bt-reasoning-world))



;; Floor spawning
(prolog:prolog '(and (btr:bullet-world ?world)
                     (assert (btr:object ?world :static-plane :floor ((0 0 0) (0 0 0 1))
                                         :normal (0 0 1) :constant 0))))

;; spawning the generate urdf building
(let ((building-urdf 
        (cl-urdf:parse-urdf 
         (roslisp:get-param "building_description"))))
  (prolog:prolog
   `(and (btr:bullet-world ?world)
         (assert (btr:object ?world :urdf :ifc-building ((0 0 0) (0 0 0 1))
                             :urdf ,building-urdf)))))

;; starting debug window
(prolog:prolog '(and (btr:bullet-world ?world) 
                     (btr:debug-window ?world)))



;; spawn a robot

(let* ((robot-urdf (cl-urdf:parse-urdf (roslisp:get-param "robot_description")))
       (quat (cl-transforms:euler->quaternion :az 0.0)))
  (prolog:prolog
   `(and (btr:bullet-world ?w)
         (assert (btr:object ?w :urdf :pr2 ((5.0 5.0 0.1)
                                            (,(cl-transforms:x quat)
                                             ,(cl-transforms:y quat)
                                             ,(cl-transforms:z quat)
                                             ,(cl-transforms:w quat)))
                             :urdf ,robot-urdf)))))
```