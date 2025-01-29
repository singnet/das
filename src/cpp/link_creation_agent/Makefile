build_docs: 
		python doc/DaC/das_link_creation_diagram.py

build:
		cd ../../ && bash scripts/build.sh && cd -

run_server:
		cd ../../ && bash scripts/container_tty.sh ./bin/link_creation_server ${OPTIONS}  && cd -

run_client:
		cd ../../ && bash scripts/container_tty.sh ./bin/link_creation_agent_client ${OPTIONS}  && cd -


tests:
		cd ../../ && bash scripts/unit_tests.sh && cd -