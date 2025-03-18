FROM ubuntu:22.04

ARG DAS_QUERY_ENGINE_BRANCH="masc/change-service-in-grpc"

RUN apt-get update && apt-get install -y \
    python3.10 \
    python3.10-distutils \
    python3.10-venv \
    python3-pip \
    binutils \
    git

RUN update-alternatives --install /usr/bin/python python /usr/bin/python3.10 1

WORKDIR /app

ENV PYTHONPATH=/app

RUN git clone https://github.com/singnet/das-query-engine.git
RUN cd das-query-engine && git checkout ${DAS_QUERY_ENGINE_BRANCH} &&  mv hyperon_das/ ../hyperon_das
RUN pip3 install --no-cache --upgrade pip hyperon_das_atomdb hyperon-das-node requests grpcio google protobuf pyinstaller

COPY . /app/evolution

COPY build.sh .
RUN chmod +x build.sh

CMD ["./build.sh"]