#!/bin/bash

# Usage: lca_integration_test.sh [-y]
#
#     -y bypasses the warning message about stopping any running docker containers

"$(dirname "$0")"/lca_test_environment.sh $1 && "$(dirname "$0")"/../../scripts/run.sh lca_integration_test

