#!/bin/bash

# Generate MeTTa file with random words

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <output_file> <sentence_count> <word_count> <word_length> <alphabet_range>"
    exit 1
fi

python3 metta_file_generator.py \
    --filename "$1" \
    --sentence-node-count "$2" \
    --word-count "$3" \
    --word-length "$4" \
    --alphabet-range "$5"

if [ $? -ne 0 ]; then
    echo "Error generating MeTTa file."
    exit 1
fi 

# Load the generated MeTTa file into the database
das-cli db start
if [ $? -ne 0 ]; then
    echo "Error starting the database."
    exit 1
fi