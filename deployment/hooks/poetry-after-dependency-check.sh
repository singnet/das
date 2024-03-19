#!/bin/bash

set -e

poetry_version="$das_release_version"
poetry_library_name=""
poetry_dependency_name=""

echo "Updating $poetry_library_name to version $poetry_version in the $poetry_dependency_name library."
poetry add $poetry_library_name@$poetry_version
