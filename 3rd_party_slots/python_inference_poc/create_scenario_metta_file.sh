#!/bin/bash
#
# This script clear useless lines (type definitions, Sentence and Words expression etc) and join the
# base and the test KBs in a single metta file named using the names of both files.
#
# Usage: create_scenario_metta_file.sh <knowledge base file name> <test file name>
# 


f1_full_name=$1
f1_name="${f1_full_name%.*}"
f2_full_name=$2
f2_name="${f2_full_name%.*}"
target_name="${f1_name}__${f2_name}.metta"
rm -f ${target_name}
grep Contains ${f1_full_name} | grep -v ":" > ${target_name}
grep EVALUATION ${f1_full_name} >> ${target_name}
grep Contains ${f2_full_name} | grep -v ":" >> ${target_name}
grep EVALUATION ${f2_full_name} >> ${target_name}
