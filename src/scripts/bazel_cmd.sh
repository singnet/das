#!/bin/bash

set -exou pipefail

CMD="/opt/bazel/bazelisk"

ARGS=$([ ${BAZEL_JOBS:-} != x ] && echo --jobs=${BAZEL_JOBS})

${CMD} ${ARGS} ${@}
