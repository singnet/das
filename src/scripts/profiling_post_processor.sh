#!/bin/sh

cat $1 | grep "#PROFILER#" | grep STOP_WATCH | cut -d" " -f12- | cut -d" " -f3- | rev | cut -d" " -f5- | rev | sed "s/\"\ /\",/g" | python3 ./src/scripts/compute_stats.py
