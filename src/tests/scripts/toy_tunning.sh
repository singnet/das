#!/bin/bash

KB=$1
CONTEXT_TAG=$2
TARGET_PREDICATE=$3
TARGET_CONCEPT=$4

log_files=("/tmp/ab.log" "/tmp/qa.log" "/tmp/ev.log" "/tmp/run_toy.log")

for LOG_FILE in "${log_files[@]}"; do
    rm -f $LOG_FILE
done

RENT_LIST=("0.2" "0.5" "0.8")
SPREAD_LIST=("0.2" "0.5" "0.8")
SELECTION_LIST=("0.03:0.10" "0.08:0.30")

for RENT in "${RENT_LIST[@]}"; do
    for SPREAD in "${SPREAD_LIST[@]}"; do
        for selection_pair in "${SELECTION_LIST[@]}"; do
            ELITISM="${selection_pair%%:*}"
            SELECTION="${selection_pair#*:}"
            command_line=(./src/tests/scripts/run_toy.sh $KB $CONTEXT_TAG "$TARGET_PREDICATE" "$TARGET_CONCEPT" $RENT $SPREAD $SPREAD $ELITISM $SELECTION)
            for LOG_FILE in "${log_files[@]}"; do
                echo "----------------------------------------------------------------------------------------------------" >> $LOG_FILE
                echo "${command_line[@]}" >> $LOG_FILE
                echo "----------------------------------------------------------------------------------------------------" >> $LOG_FILE
                echo "" >> $LOG_FILE
            done
            "${command_line[@]}" 2>&1 | tee /tmp/run_toy.log | grep FINAL_RESULT
        done
    done
done
