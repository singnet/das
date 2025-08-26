#!/bin/bash
# set -eoux pipefail
PWD=$(dirname "$0")
JSON_FILE=$1
METTA_FILE=$2
METTA_BIAS_FILE=$3
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
rm -f /tmp/output.metta
rm -f /tmp/biased_predicates.metta
if [ -z "$METTA_FILE" ]; then
  rm -f 3rd_party_slots/python_inference_poc/output.metta
  $PWD/run_script.sh python3 $ITPPATH/metta_file_generator.py --sentence-node-count $SENTENCE_NODE_COUNT --word-count $WORD_COUNT --word-length $WORD_LENGTH --alphabet-range $ALPHABET
  cp -f $ITPPATH/output.metta /tmp/
else
  cp -f "$METTA_FILE" /tmp/output.metta
fi

if [ -z "$METTA_BIAS_FILE" ]; then
  rm -f 3rd_party_slots/python_inference_poc/biased_predicates.metta
  $PWD/run_script.sh python3 $ITPPATH/predicate_generator.py  $ITPPATHD/biased_predicates.metta --config-file $ITPPATHD/config/$SCENARIO.json
  cp -f $ITPPATH/biased_predicates.metta /tmp/
else
  cp -f "$METTA_BIAS_FILE" /tmp/biased_predicates.metta
fi
das-cli db start
$PWD/run_agents.sh start no-wait
$PWD/run_agents.sh stop
das-cli metta load /tmp/output.metta
das-cli metta load /tmp/biased_predicates.metta
export LINK_CREATION_REQUESTS_INTERVAL_SECONDS=5
$PWD/run_agents.sh start