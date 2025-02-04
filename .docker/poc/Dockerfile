FROM trueagi/das:22.04-builder AS builder

ENV HOME=/opt/das
ENV BIN_DIR=${HOME}/bin \
    CPP_WORKSPACE_DIR=${HOME}/cpp \
    PYTHON_WORKSPACE_DIR=${HOME}/python

WORKDIR ${HOME}

COPY ./src .

RUN chmod +x ./scripts/*.sh 

RUN ./scripts/bazel_build.sh --binaries

# Build Attention Broker
FROM trueagi/das:22.04-builder AS attention-broker

COPY --from=builder /opt/das/bin/attention_broker_service /usr/local/bin

ENTRYPOINT [ "attention_broker_service" ]

# Build Query Agent
FROM trueagi/das:22.04-builder AS query-agent

COPY --from=builder /opt/das/bin/query_broker /usr/local/bin

ENTRYPOINT [ "query_broker" ]

# Build Link Creation Agent
FROM trueagi/das:22.04-builder AS link-creation-agent

COPY --from=builder /opt/das/bin/link_creation_server /usr/local/bin

ENTRYPOINT [ "link_creation_server" ]

# Build Link Creation Agent
FROM trueagi/das:22.04-builder AS link-creation-client

COPY --from=builder /opt/das/bin/link_creation_agent_client /usr/local/bin

ENTRYPOINT [ "link_creation_agent_client" ]
