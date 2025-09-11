#!/bin/bash -x

set -euo pipefail

declare -A TITLE_ALIAS_MAP=(
  ["morkdb"]="MorkDB"
  ["redismongodb"]="Redis + MongoDB"
  ["queryagent"]="Query Agent"
)

TITLE="$2"
BENCHMARK_DATABASE_PATH="/home/$USER/.cache/shared/benchmark.db"

function get_pr_info() {
  curl -s -H "Authorization: token $GITHUB_TOKEN" \
       -H "Accept: application/vnd.github.groot-preview+json" \
       "https://api.github.com/repos/${GITHUB_REPOSITORY}/commits/${GITHUB_SHA}/pulls"
}

function get_commit_info() {
  curl -s -H "Authorization: token $GITHUB_TOKEN" \
       "https://api.github.com/repos/${GITHUB_REPOSITORY}/commits/${GITHUB_SHA}"
}

function format_title() {
  local prefix="$1"
  if [[ -n "${TITLE_ALIAS_MAP[$prefix]:-}" ]]; then
    echo "${TITLE_ALIAS_MAP[$prefix]}"
  else
    echo "$prefix" | tr '_' ' ' | sed 's/\b\(.\)/\u\1/g'
  fi
}

function get_prefixes() {
  echo "$1" | jq -r '.[].backend' | sort -u
}

function build_metadata_section() {
  local pr_info commit_info pr_url pr_number pr_title pr_base_branch commit_date commit_date_fmt commit_sha_short

  pr_info=$(get_pr_info)
  commit_info=$(get_commit_info)

  pr_url=$(echo "$pr_info" | jq -r '.[0].html_url // empty')
  pr_number=$(echo "$pr_info" | jq -r '.[0].number // empty')
  pr_title=$(echo "$pr_info" | jq -r '.[0].title // empty')
  pr_base_branch=$(echo "$pr_info" | jq -r '.[0].base.ref // "master"')

  commit_date=$(echo "$commit_info" | jq -r '.commit.author.date')
  commit_date_fmt=$(date -u -d "$commit_date" +"%Y-%m-%d %H:%M UTC")
  commit_sha_short=$(echo "$GITHUB_SHA" | cut -c1-7)

  echo "## $TITLE"$'\n'
  echo "**Repository:** $GITHUB_REPOSITORY"
  if [[ "$pr_url" != "" ]]; then
    echo "**Source:** [#$pr_number - $pr_title]($pr_url)"
  fi
  echo "**Date:** $commit_date_fmt"

  if [[ "$pr_url" != "" ]]; then
    echo "**Commit:** \`$commit_sha_short\` ($pr_base_branch)"
  else
    echo "**Commit:** \`$commit_sha_short\`"
  fi
}

function build_table_for_prefix() {
  local benchmark_result="$1"
  local prefix="$2"
  local window="${3:-10}"
  local threshold="${4:-10}"
  local title

  title=$(format_title "$prefix")
  echo $'\n\n'"### $title"$'\n'

  echo "$benchmark_result" \
    | jq --arg prefix "$prefix" '[.[] | select(.backend == $prefix)]' \
    | jq -c '.[]' | while read -r row; do
        op=$(echo "$row" | jq -r '.operation')
        median=$(echo "$row" | jq -r '.median_operation_time_ms')
        throughput=$(echo "$row" | jq -r '.throughput')
        tpa=$(echo "$row" | jq -r '.time_per_atom_ms')

        history=$(get_history_for_operation "$op" "$prefix" "$window")

        hist_median_avg=$(echo "$history" | awk '{sum+=$1} END {if(NR>0) print sum/NR; else print 0}')
        hist_tpa_avg=$(echo "$history" | awk '{sum+=$2} END {if(NR>0) print sum/NR; else print 0}')
        hist_throughput_avg=$(echo "$history" | awk '{sum+=$2} END {if(NR>0) print sum/NR; else print 0}')

        if [ "$hist_median_avg" != "0" ] && (( $(echo "$median > $hist_median_avg * (1 + $threshold/100)" | bc -l) )); then
          median=":red_circle: $median"
        fi

        if [ "$hist_tpa_avg" != "0" ] && (( $(echo "$tpa > $hist_tpa_avg * (1 + $threshold/100)" | bc -l) )); then
          tpa=":red_circle: $tpa"
        fi

        if [ "$hist_throughput_avg" != "0" ] && (( $(echo "$throughput < $hist_throughput_avg * (1 - $threshold/100)" | bc -l) )); then
          throughput=":red_circle: $throughput"
        fi

        echo "$op,$median,$tpa,$throughput"
    done \
    | mlr --ocsv cat \
    | mlr --icsv --omd rename 1,Operation,2,Median,3,"Time Per Atom",4,Throughput
}

function get_benchmark_result_by_id() {
  local benchmark_id="$1"

  sqlite3 $BENCHMARK_DATABASE_PATH "SELECT
    '[' || GROUP_CONCAT(
        json_object(
            'id', id,
            'benchmark_execution_id', benchmark_execution_id,
            'backend', backend,
            'operation', operation,
            'batch_size', batch_size,
            'median_operation_time_ms', median_operation_time_ms,
            'min_operation_time_ms', min_operation_time_ms,
            'max_operation_time_ms', max_operation_time_ms,
            'p50_operation_time_ms', p50_operation_time_ms,
            'p90_operation_time_ms', p90_operation_time_ms,
            'p99_operation_time_ms', p99_operation_time_ms,
            'total_time_ms', total_time_ms,
            'time_per_atom_ms', time_per_atom_ms,
            'throughput', throughput
        ), 
        ','
    ) || ']'
  AS results
  FROM benchmark_result
  WHERE benchmark_execution_id = $benchmark_id;" | head -n 1
}

function get_history_for_operation() {
  local operation="$1"
  local backend="$2"
  local limit="${3:-10}"

  sqlite3 $BENCHMARK_DATABASE_PATH "SELECT
    median_operation_time_ms,
    throughput
  FROM benchmark_result
  WHERE backend = '$backend'
    AND operation = '$operation'
  ORDER BY id DESC
  LIMIT $limit;"
}

function generate_message() {
  local benchmark_result="$1"
  local message
  message="$(build_metadata_section)"
  for prefix in $(get_prefixes "$benchmark_result"); do
    message+=$(build_table_for_prefix "$benchmark_result" "$prefix")
  done
  echo "$message"
}

function update_benchmark_execution() {
  local benchmark_id="$1"
  local completed_at="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"

  local pr_info pr_url pr_title 

  pr_info=$(get_pr_info)
  commit_info=$(get_commit_info)

  pr_link=$(echo "$pr_info" | jq -r '.[0].html_url // empty')
  pr_title=$(echo "$pr_info" | jq -r '.[0].title // empty')

  sqlite3 "$BENCHMARK_DATABASE_PATH" <<EOF
UPDATE benchmark_execution
SET status = 'COMPLETED',
    pr_link = '$pr_link',
    pr_title = '$pr_title',
    completed_at = '$completed_at'
WHERE id = $benchmark_id;
EOF
}

function save_message_as_json() {
  local msg="$1"
  local escaped
  escaped=$(jq -Rs . <<< "$msg")
  echo "{\"text\": $escaped}" > mattermost.json
}

function main() {
  local message benchmark_result
  local benchmark_id="$1"

  benchmark_result=$(get_benchmark_result_by_id "$benchmark_id")

  if [ -z "$benchmark_result" ]; then
    echo "Benchmark result could not be found in the database"
    exit 1
  fi

  message=$(generate_message "$benchmark_result")
  echo "$message"
  save_message_as_json "$message"

  update_benchmark_execution "$benchmark_id"
}

main "$@"
