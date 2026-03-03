(defsystem cram-ifc-building
  :depends-on (roslisp-utilities
               cl-urdf
               
               cl-transforms
               cl-transforms-stamped
               cl-tf
               cram-tf

               cram-language
               cram-prolog
               
               cram-physics-utils
               cl-bullet
               cram-bullet-reasoning
               cram-bullet-reasoning-belief-state
               cram-bullet-reasoning-utilities
               ifc_tree_to_graph-srv)
  
  :components
  ((:module "lisp"
    :components
    ((:file "package")
     (:file "ifc-building" :depends-on ("package"))))))
