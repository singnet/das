build_docs: 
		python doc/DaC/das_link_creation_diagram.py

build:
		cd ../ && bazel build //:link_creation_server --incompatible_disallow_empty_glob=false && cd -


run:
		cd ../ && ./bazel-bin/link_creation_server.runfiles/_main/link_creation_server && cd -