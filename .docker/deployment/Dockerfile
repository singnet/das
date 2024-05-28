FROM ubuntu:22.04

WORKDIR /opt

COPY ./scripts ./scripts

RUN apt -y update && \
    apt -y install jq git curl && \
    chmod +x ./scripts/deployment -R

SHELL [ "/bin/bash" ]
