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
               cram-bullet-reasoning-utilities)
  
  :components
  ((:module "lisp"
    :components
    ((:file "package")
     (:file "setup" :depends-on ("package"))
     (:file "navigation" :depends-on ("package" "setup"))
     (:file "ifc-building" :depends-on ("package" "setup"))))))
