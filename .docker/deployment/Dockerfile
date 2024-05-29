FROM ubuntu:22.04

WORKDIR /opt

COPY ./scripts ./scripts

RUN apt -y update && \
    apt -y install jq git curl python3-pip && \
    pip3 install poetry && \
    chmod +x ./scripts/deployment -R

SHELL [ "/bin/bash" ]
