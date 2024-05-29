FROM ubuntu:22.04

WORKDIR /opt

RUN apt -y update && \
    apt -y install jq git curl python3-pip && \
    pip3 install poetry

COPY ./scripts ./scripts

RUN chmod +x ./scripts/deployment -R

SHELL [ "/bin/bash" ]
