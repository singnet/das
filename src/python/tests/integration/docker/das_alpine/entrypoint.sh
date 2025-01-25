#! /bin/bash
dockerd &
python3 das-toolbox/src/das_cli.py db start && \
python3 das-toolbox/src/das_cli.py metta load /das/animals.metta && \
python3 das-toolbox/src/das_cli.py faas start && \
sleep infinity