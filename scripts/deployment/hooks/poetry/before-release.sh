#!/bin/bash

set -e

local poetry_version="$new_package_version"
local package_folder_path="${package_name//-/_}"
local init_file_path=$package_folder_path/__init__.py

poetry version $poetry_version

if [ -f "$init_file_path" ]; then
    sed_inplace "s/__version__ = .*/__version__ = '$poetry_version'/g" "$package_folder_path/__init__.py"
fi
