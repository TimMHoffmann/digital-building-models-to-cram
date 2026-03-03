(in-package :ifc-tree-to-graph)

(defparameter *task-path-service* "/ifc_tree_to_graph/get_task_path")

(defun call-get-task-path (task-name)
  "Calls the IFC task-path service by task name. Returns the ROS response object."
  (let ((service (roslisp:wait-for-service *task-path-service* 10.0)))
    (unless service
      (error "Service ~a not available" *task-path-service*))
    (roslisp:call-service
     *task-path-service*
     'ifc_tree_to_graph-srv:GetTaskPath
     :task_name task-name)))

(defun get-task-path (task-name)
  "Convenience wrapper. Returns values: success, message, task-name, start-space-id, end-space-id, path-nodes, space-ids, space-points."
  (let ((resp (call-get-task-path task-name)))
    (values (roslisp:msg-slot-value resp 'success)
            (roslisp:msg-slot-value resp 'message)
            (roslisp:msg-slot-value resp 'task_name)
            (roslisp:msg-slot-value resp 'start_space_id)
            (roslisp:msg-slot-value resp 'end_space_id)
            (roslisp:msg-slot-value resp 'path_nodes)
            (roslisp:msg-slot-value resp 'space_ids)
            (roslisp:msg-slot-value resp 'space_points))))