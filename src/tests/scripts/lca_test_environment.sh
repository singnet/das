#!/bin/bash

# Usage: lca_test_environment.sh [-y]
#
#     -y bypasses the warning message about stopping any running docker containers

if [[ "$*" != *"-y"* ]]; then
    while true; do
        read -p "Any running docker containers will be stopped/pruned. Do you want to proceed? (y/n): " yn
        case $yn in
            [Yy]* )
                echo "Proceeding..."
                break
                ;;
            [Nn]* )
                echo "Operation canceled."
                exit 1
                ;;
            * )
                echo "Please answer yes (y) or no (n)."
                ;;
        esac
    done
fi

if ! command -v das-cli &>/dev/null; then
    echo "ERROR: Can't find das-cli"
    exit 1
fi

TMP_DIR=`mktemp -d -p /tmp`
KBGZ="inference_toy_500_5_3_abcde_50_40.metta.gz"
KB="${KBGZ%.*}"
cp "$(dirname "$0")"/../assets/$KBGZ ${TMP_DIR}
gunzip --quiet --force ${TMP_DIR}/$KBGZ
das-cli db stop --prune >> ${TMP_DIR}/atomdb.log 2>&1
(docker stop `docker ps | tail -n +2 | cut -d" " -f1` ; docker container prune -f ; docker ps -a) > /dev/null 2>&1
das-cli db start >> ${TMP_DIR}/atomdb.log 2>&1
make run-db-loader OPTIONS="--config=/opt/das/config/das.json --file=${TMP_DIR}/$KB --threads=4 --chunk=5000" >> ${TMP_DIR}/atomdb.log 2>&1
make run-attention-broker >> ${TMP_DIR}/attention_broker.log 2>&1 &
sleep 5
make run-busnode OPTIONS="--service=query-engine --config=/opt/das/config/das.json" >> ${TMP_DIR}/query_engine.log 2>&1 &
sleep 5
make run-busnode OPTIONS="--service=link-creation-agent --bus-endpoint=localhost:40002 --config=/opt/das/config/das.json" >> ${TMP_DIR}/link_creation_agent.log 2>&1 &
sleep 5
\rm -rf ${TMP_DIR}
exit 0
