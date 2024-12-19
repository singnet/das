FROM ubuntu:22.04

ARG BASE_DIR="/opt"
ARG TMP_DIR="/tmp"

ARG ATTENTION_BROKER_DIR="${BASE_DIR}/das-attention-broker"
ARG DATA_DIR="${BASE_DIR}/data"
ARG GRPC_DIR="${BASE_DIR}/grpc"
ARG PROTO_DIR="${BASE_DIR}/proto"
ARG BAZEL_DIR="${BASE_DIR}/bazel"
ARG THIRDPARTY="${BASE_DIR}/3rd-party"

RUN mkdir -p ${ATTENTION_BROKER_DIR}
RUN mkdir -p ${DATA_DIR}
RUN mkdir -p ${GRPC_DIR}
RUN mkdir -p ${BAZEL_DIR}
RUN mkdir -p ${PROTO_DIR}
RUN mkdir -p ${THIRDPARTY}
#VOLUME ${ATTENTION_BROKER_DIR}

RUN apt-get update -y
RUN apt-get install -y git

RUN cd ${GRPC_DIR} &&\
    git clone https://github.com/grpc/grpc &&\
    cd grpc &&\
    git submodule update --init

RUN apt-get install -y build-essential
RUN apt-get install -y autoconf
RUN apt-get install -y libtool
RUN apt-get install -y pkg-config
RUN apt-get install -y curl
RUN apt-get install -y gcc
RUN apt-get install -y protobuf-compiler
#RUN apt-get install -y libmbedcrypto7
RUN apt-get install -y libmbedtls14
RUN apt-get install -y libmbedtls-dev
RUN apt-get install -y libevent-dev
RUN apt-get install -y libssl-dev

COPY assets/3rd-party.tgz ${THIRDPARTY}
RUN cd ${THIRDPARTY} &&\
    tar xzvf 3rd-party.tgz &&\
    rm -f 3rd-party.tgz &&\
    mkdir -p ${ATTENTION_BROKER_DIR}/src/3rd-party &&\
    ln -s ${THIRDPARTY} ${ATTENTION_BROKER_DIR}/src/3rd-party &&\
    mv bazelisk ${BAZEL_DIR}
    
ENV CPLUS_INCLUDE_PATH="/opt/3rd-party/mbedcrypto/include/"

ENV CC=/usr/bin/gcc
RUN ln -s ${BAZEL_DIR}/bazelisk /usr/bin/bazel
RUN cd ${GRPC_DIR}/grpc &&\
    ${BAZEL_DIR}/bazelisk build :all

ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/attention_broker.proto ${PROTO_DIR}
ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/common.proto ${PROTO_DIR}
ADD https://raw.githubusercontent.com/singnet/das-query-engine/master/proto/echo.proto ${PROTO_DIR}

################################################################################
# To be removed when AtomDB is properly integrated
# Redis client
RUN apt-get install -y cmake
RUN apt-get install -y libevent-dev
RUN apt-get install -y libssl-dev
RUN apt-get install -y pkg-config
RUN apt-get install -y cmake-data
COPY assets/hiredis-cluster.tgz /tmp
COPY assets/mongo-cxx-driver-r3.11.0.tar.gz /tmp
RUN cd /tmp &&\
    tar xzf hiredis-cluster.tgz &&\
    cd hiredis-cluster &&\
    mkdir build &&\
    cd build &&\
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_SSL=ON ..&&\
    make &&\
    make install &&\
    echo "/usr/local/lib" > /etc/ld.so.conf.d/local.conf &&\
    ldconfig
# MongoDB client
RUN cd /tmp &&\
    tar xzvf mongo-cxx-driver-r3.11.0.tar.gz &&\
    cd /tmp/mongo-cxx-driver-r3.11.0/build/ &&\
    cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF &&\
    cmake --build . &&\
    cmake --build . --target install && \
#    mv install/include/* /usr/local/include &&\
    ln -s /usr/local/include/bsoncxx/v_noabi/bsoncxx/* /usr/local/include/bsoncxx &&\
    ln -s /usr/local/include/bsoncxx/v_noabi/bsoncxx/third_party/mnmlstc/core/ /usr/local/include/core &&\
    ln -s /usr/local/include/mongocxx/v_noabi/mongocxx/* /usr/local/include/mongocxx/ &&\
#    mv install/lib/* /usr/local/lib &&\
    ldconfig

################################################################################


WORKDIR /opt/das-attention-broker
