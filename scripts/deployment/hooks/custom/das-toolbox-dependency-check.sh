#!/bin/bash

set -e

local config_file_path="das-cli/src/settings/config.py"

if ! source "$hooks_path/common/dependency-check.sh"; then
    return 0
fi

if ! verify_file_exists $config_file_path; then
    print ":red:$config_file_path file not found.:/red:"
    exit 1
fi

if [ "$dependency" == "das-metta-parser" ]; then
    if grep -q "^METTA_PARSER_IMAGE_VERSION =" $config_file_path; then
        sed_inplace "s/^METTA_PARSER_IMAGE_VERSION = \".*\"/METTA_PARSER_IMAGE_VERSION = \"${dependency_new_version}-metta-parser\"/" "$config_file_path"
        print ":green:Package ${dependency} version updated to ${dependency_new_version} in $config_file_path file.:/green:"
    else
        print ":red:Variable 'METTA_PARSER_IMAGE_VERSION' not found in the $config_file_path file.:/red:"
    fi
else
    print ":yellow:Dependency $dependency action not set up in the dependency-check script:/yellow:"
fi
