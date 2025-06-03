#!/bin/bash

set -e

local file_path="das-cli/src/settings/config.py"

if [ -f "$file_path" ]; then
    sed_inplace "s/^VERSION = .*/VERSION = '$new_package_version'/g" "$file_path"
fi
