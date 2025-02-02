#!/bin/bash -x

BIN_DIR="./bin"

# if [ ! -f "$BIN_DIR/sentinel_server" ] || [ ! -f "$BIN_DIR/worker_client" ]; then
#     echo "'sentinel_server' and/or 'worker_client' files not found in $BIN_DIR."
echo "Running 'docker_image_build.sh' and 'build.sh'..."

$(pwd)/scripts/docker_image_build.sh

if [ $? -ne 0 ]; then
    echo "Error running 'docker_image_build.sh'. Aborting."
    exit 1
fi

$(pwd)/scripts/build.sh

if [ $? -ne 0 ]; then
    echo "Error running 'bazel_build.sh'. Aborting."
    exit 1
fi

echo "Build completed successfully."
# else
#     echo "'sentinel_server' and 'worker_client' files already exist in $BIN_DIR. "
# fi

docker buildx build -t das-spawner-client-builder --load -f docker/Dockerfile.spawner .