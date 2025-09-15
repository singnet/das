#!/bin/bash

echo "--------------------------------------------------------------------------------" &&\
echo "Stopping agents" &&\
echo "--------------------------------------------------------------------------------" &&\
das-cli inference-agent stop
das-cli evolution-agent stop
das-cli link-creation-agent stop
das-cli query-agent stop
das-cli ab stop
