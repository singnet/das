FROM ubuntu:22.04

ARG DAS_QUERY_ENGINE_BRANCH="masc/change-service-in-grpc"

RUN apt-get update && apt-get install -y \
    python3.10 \
    python3.10-distutils \
    python3.10-venv \
    python3-pip \
    binutils \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/python python /usr/bin/python3.10 1

WORKDIR /app

ENV PYTHONPATH=/app

RUN apt-get update && apt-get install -y git

RUN git clone https://github.com/singnet/das-query-engine.git
RUN cd das-query-engine && git checkout ${DAS_QUERY_ENGINE_BRANCH} &&  mv hyperon_das/ ../hyperon_das
RUN pip3 install --no-cache --upgrade pip hyperon_das_atomdb requests grpcio google protobuf pyinstaller

COPY asset/hyperon_das_node-0.0.1-cp310-abi3-manylinux_2_28_x86_64.whl /app/
RUN pip3 install /app/hyperon_das_node-0.0.1-cp310-abi3-manylinux_2_28_x86_64.whl

COPY . /app/evolution

COPY build.sh .
RUN chmod +x build.sh

CMD ["./build.sh"]