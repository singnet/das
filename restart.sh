#!/bin/bash

echo "--------------------------------------------------------------------------------" &&\
echo "Starting agents" &&\
echo "--------------------------------------------------------------------------------" &&\
das-cli ab restart && sleep 2 &&\
das-cli query-agent restart --port-range 42000:42999 && sleep 2 &&\
#das-cli link-creation-agent restart --port-range 43000:43999 --peer-hostname localhost --peer-port 40002 && sleep 2 &&\
#das-cli inference-agent restart --port-range 44000:44999 --peer-hostname localhost --peer-port 40002 && sleep 2 &&\
das-cli evolution-agent restart --port-range 45000:45999 && sleep 2 &&\
CONTAINER_AB=`docker ps | grep "trueagi/das:attention-broker-" | cut -d" " -f1` &&\
CONTAINER_QA=`docker ps | grep "trueagi/das:query-agent-" | cut -d" " -f1` &&\
#CONTAINER_LCA=`docker ps | grep "trueagi/das:link-creation-agent-" | cut -d" " -f1` &&\
#CONTAINER_INF=`docker ps | grep "trueagi/das:inference-agent-" | cut -d" " -f1` &&\
CONTAINER_EV=`docker ps | grep "trueagi/das:evolution-agent-" | cut -d" " -f1` &&\
echo "Attention Broker: ${CONTAINER_AB}"
echo "Query agent: ${CONTAINER_QA}"
#echo "LCA: ${CONTAINER_LCA}"
#echo "Inference: ${CONTAINER_INF}"
echo "Evolution: ${CONTAINER_EV}"
\
echo "--------------------------------------------------------------------------------" &&\
echo "Reseting executables" &&\
echo "--------------------------------------------------------------------------------" &&\
echo "bin/attention_broker_service -> ${CONTAINER_AB}:/usr/local/bin" &&\
docker cp bin/attention_broker_service ${CONTAINER_AB}:/usr/local/bin &&\
echo "bin/query_broker -> ${CONTAINER_QA}:/usr/local/bin" &&\
docker cp bin/query_broker ${CONTAINER_QA}:/usr/local/bin &&\
#echo "bin/link_creation_server -> ${CONTAINER_LCA}:/usr/local/bin" &&\
#docker cp bin/link_creation_server ${CONTAINER_LCA}:/usr/local/bin &&\
#echo "bin/inference_agent_server -> ${CONTAINER_INF}:/usr/local/bin" &&\
#docker cp bin/inference_agent_server ${CONTAINER_INF}:/usr/local/bin &&\
echo "bin/evolution_broker -> ${CONTAINER_EV}:/usr/local/bin" &&\
docker cp bin/evolution_broker ${CONTAINER_EV}:/usr/local/bin &&\
\
echo "--------------------------------------------------------------------------------" &&\
echo "Restarting containers" &&\
echo "--------------------------------------------------------------------------------" &&\
docker restart ${CONTAINER_AB} && sleep 2 &&\
docker restart ${CONTAINER_QA} && sleep 2 &&\
#docker restart ${CONTAINER_LCA} && sleep 2 &&\
#docker restart ${CONTAINER_INF} && sleep 2 &&\
docker restart ${CONTAINER_EV} && sleep 2 &&\
\
echo "--------------------------------------------------------------------------------" &&\
echo "DONE"
