FROM ubuntu:22.04

ARG BASE_DIR="/opt"
ARG TMP_DIR="/tmp"

ARG DAS_DIR="${BASE_DIR}/das"
ARG DATA_DIR="${BASE_DIR}/data"
ARG GRPC_DIR="${BASE_DIR}/grpc"
ARG PROTO_DIR="${BASE_DIR}/proto"
ARG BAZEL_DIR="${BASE_DIR}/bazel"
ARG THIRD_PARTY="${BASE_DIR}/3rd-party"

RUN mkdir -p \
    ${DAS_DIR} \
    ${DATA_DIR} \
    ${GRPC_DIR} \
    ${BAZEL_DIR} \
    ${PROTO_DIR} \
    ${THIRD_PARTY}

RUN yes | apt update -y \
    && yes | apt install -y git build-essential curl protobuf-compiler python3 python3-pip cmake \
    libevent-dev libssl-dev pkg-config libncurses5\
    && yes | apt clean -y \
    && rm -rf /var/lib/apt/lists/*

COPY src/assets/bazelisk.tgz /tmp
RUN ARCH=$([ "$(uname -m)" = "aarch64" ] && echo "arm64" || echo "amd64") \
    && cd /tmp \
    && tar xzvf bazelisk.tgz \
    && mv bazelisk-${ARCH} ${BAZEL_DIR}/bazelisk \
    && mv buildifier-${ARCH} ${BAZEL_DIR}/buildifier \
    && rm -f bazelisk*

################################################################################
# To be removed when AtomDB is properly integrated
# Redis client
COPY src/assets/hiredis-cluster.tgz /tmp
COPY src/assets/mongo-cxx-driver-r3.11.0.tar.gz /tmp
RUN cd /tmp \
    && tar xzf hiredis-cluster.tgz \
    && cd hiredis-cluster \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_SSL=ON .. \
    && make \
    && make install \
    && echo "/usr/local/lib" > /etc/ld.so.conf.d/local.conf \
    && ldconfig
# MongoDB client
RUN cd /tmp \
    && tar xzvf mongo-cxx-driver-r3.11.0.tar.gz \
    && cd /tmp/mongo-cxx-driver-r3.11.0/build/ \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF \
    && cmake --build . \
    && cmake --build . --target install \
    && ln -s /usr/local/include/bsoncxx/v_noabi/bsoncxx/* /usr/local/include/bsoncxx \
    && ln -s /usr/local/include/bsoncxx/v_noabi/bsoncxx/third_party/mnmlstc/core/ /usr/local/include/core \
    && ln -s /usr/local/include/mongocxx/v_noabi/mongocxx/* /usr/local/include/mongocxx/ \
    && ldconfig

# add regular user
RUN useradd -ms /bin/bash builder

################################################################################


WORKDIR ${DAS_DIR}
