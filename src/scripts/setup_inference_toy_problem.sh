#!/bin/bash
# set -eoux pipefail
PWD=$(dirname "$0")
JSON_FILE=$1
echo "Using JSON file: $JSON_FILE"
# extract the scenario from json file name
SCENARIO=$(basename "$JSON_FILE" .json)
SENTENCE_NODE_COUNT=$(jq -r '."sentence-node-count"' "$JSON_FILE")
WORD_COUNT=$(jq -r '."word-count"' "$JSON_FILE")
WORD_LENGTH=$(jq -r '."word-length"' "$JSON_FILE")
ALPHABET=$(jq -r '."alphabet-range"' "$JSON_FILE")
das-cli db stop
ITPPATH="3rd_party_slots/python_inference_poc"
ITPPATHD="/opt/das/3rd_party_slots/python_inference_poc"
set -eoux pipefail
rm -f /tmp/output.metta
rm -f /tmp/biased_predicates.metta
rm -f 3rd_party_slots/python_inference_poc/output.metta
rm -f 3rd_party_slots/python_inference_poc/biased_predicates.metta
$PWD/run_script.sh python3 $ITPPATH/metta_file_generator.py --sentence-node-count $SENTENCE_NODE_COUNT --word-count $WORD_COUNT --word-length $WORD_LENGTH --alphabet-range $ALPHABET
$PWD/run_script.sh python3 $ITPPATH/predicate_generator.py  $ITPPATHD/output.metta --config-file $ITPPATHD/config/$SCENARIO.json
cp -f $ITPPATH/output.metta $ITPPATH/biased_predicates.metta /tmp/
das-cli db start
das-cli metta load /tmp/output.metta
das-cli metta load /tmp/biased_predicates.metta
export LINK_CREATION_REQUESTS_INTERVAL_SECONDS=5
$PWD/run_agents.sh start