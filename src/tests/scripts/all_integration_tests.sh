#!/bin/bash

# Usage: all_integration_tests.sh [-y]
#
#     -y bypasses the warning message about stopping any running docker containers

"$(dirname "$0")"/lca_integration_test.sh $1
