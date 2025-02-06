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

RUN apt update -y \
    && apt install -y git build-essential curl protobuf-compiler python3 python3-pip python2 cmake \
    libevent-dev libssl-dev pkg-config libmbedtls14 libmbedtls-dev \
    && apt clean -y \
    && rm -rf /var/lib/apt/lists/*

RUN curl -L -o ${THIRD_PARTY}/mbedcrypto.tgz https://github.com/ARMmbed/mbed-crypto/archive/refs/tags/mbedcrypto-3.1.0.tar.gz
RUN cd ${THIRD_PARTY} \
    && tar xzvf mbedcrypto.tgz \
    && rm -f mbedcrypto.tgz \
    && cd mbed-crypto-mbedcrypto-3.1.0 && make \
    && cd ${THIRD_PARTY} \
    && mkdir mbedcrypto \
    && cp -r mbed-crypto-mbedcrypto-3.1.0/include mbedcrypto/ \
    && cp mbed-crypto-mbedcrypto-3.1.0/library/libmbedcrypto.a mbedcrypto/ \
    && rm -rf mbed-crypto-mbedcrypto-3.1.0/ \
    && mkdir -p ${DAS_DIR}/src/3rd-party \
    && ln -s ${THIRD_PARTY} ${DAS_DIR}/src/3rd-party

ENV CPLUS_INCLUDE_PATH="/opt/3rd-party/mbedcrypto/include/"

RUN ARCH=$(if [ "$(uname -m)" = "aarch64" ]; then echo "arm64"; else echo "amd64"; fi) \
    && curl -L -o ${BAZEL_DIR}/bazelisk https://github.com/bazelbuild/bazelisk/releases/download/v1.25.0/bazelisk-linux-${ARCH} \
    && chmod a+x ${BAZEL_DIR}/bazelisk

# TODO: integrate this to bazel build (https://github.com/singnet/das/issues/197)
ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/attention_broker.proto ${PROTO_DIR}
ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/common.proto ${PROTO_DIR}
ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/echo.proto ${PROTO_DIR}

################################################################################
# To be removed when AtomDB is properly integrated
# Redis client
COPY assets/hiredis-cluster.tgz /tmp
COPY assets/mongo-cxx-driver-r3.11.0.tar.gz /tmp
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
