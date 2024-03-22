#!/bin/bash

set -e

local poetry_version="$new_package_version"

poetry version $poetry_version
