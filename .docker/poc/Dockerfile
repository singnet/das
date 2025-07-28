# Build Attention Broker
FROM trueagi/das:22.04-builder AS attention-broker

COPY ./bin/attention_broker_service /usr/local/bin

RUN chmod +x /usr/local/bin/attention_broker_service

COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "attention_broker_service" ]

# Build Query Agent
FROM trueagi/das:22.04-builder AS query-agent

COPY ./bin/query_broker /usr/local/bin

RUN chmod +x /usr/local/bin/query_broker

COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "query_broker" ]

# Build Link Creation Agent
FROM trueagi/das:22.04-builder AS link-creation-agent

COPY ./bin/link_creation_server /usr/local/bin

RUN chmod +x /usr/local/bin/link_creation_server

COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "link_creation_server" ]

# Build Link Creation Agent
FROM trueagi/das:22.04-builder AS link-creation-client

COPY  ./bin/link_creation_agent_client /usr/local/bin

RUN chmod +x /usr/local/bin/link_creation_agent_client


COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "link_creation_agent_client" ]

# Build Inference Agent
FROM trueagi/das:22.04-builder AS inference-agent

COPY  ./bin/inference_agent_server /usr/local/bin

RUN chmod +x /usr/local/bin/inference_agent_server

COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "inference_agent_server" ]

# Build evolution broker
FROM trueagi/das:22.04-builder AS evolution-agent

COPY  ./bin/evolution_broker /usr/local/bin

RUN chmod +x /usr/local/bin/evolution_broker

COPY ./lib /usr/local/lib

RUN ldconfig

ENTRYPOINT [ "evolution_broker" ]
