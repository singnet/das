FROM python:3.10-slim

WORKDIR /app

RUN apt-get update \
    && apt-get install -y \
    git curl build-essential rsync \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV POETRY_VERSION=1.8.3
RUN curl -sSL https://install.python-poetry.org | python3 - \
    && ln -s /root/.local/bin/poetry /usr/local/bin/poetry

ENV POETRY_VIRTUALENVS_IN_PROJECT=false

# Create the proto directory and download the .proto files
RUN mkdir -p /proto_files \
    && curl -o /proto_files/atom_space_node.proto https://raw.githubusercontent.com/singnet/das-proto/refs/heads/master/atom_space_node.proto \
    && curl -o /proto_files/common.proto https://raw.githubusercontent.com/singnet/das-proto/refs/heads/master/common.proto

RUN useradd -ms /bin/bash builder

CMD [ "bash" ]