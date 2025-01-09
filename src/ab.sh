#!/bin/bash

rm -f ./bin/* && ./scripts/build.sh && ./bin/attention_broker_service 37007
