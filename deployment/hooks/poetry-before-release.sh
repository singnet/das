#!/bin/bash

set -e

poetry_version="$das_release_version"

poetry version $poetry_version

