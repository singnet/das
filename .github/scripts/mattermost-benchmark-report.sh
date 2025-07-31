#!/bin/bash
set -euo pipefail

declare -A TITLE_ALIAS_MAP=(
  ["atomdb_morkdb"]="MorkDB"
  ["atomdb_redismongodb"]="Redis + MongoDB"
)

REPORT_DIR="atomdb_benchmark"
TITLE="AtomDB Benchmark Report - Single Thread, 100 iterations"

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
  find "$REPORT_DIR" -type f -name '*.txt' \
    | sed -E 's|.*/([^_]+_[^_]+)_.*|\1|' | sort -u
}

function build_metadata_section() {
  local pr_info commit_info pr_url pr_number pr_title pr_base_branch commit_date commit_date_fmt commit_sha_short

  pr_info=$(get_pr_info)
  commit_info=$(get_commit_info)

  pr_url=$(echo "$pr_info" | jq -r '.[0].html_url // empty')
  pr_number=$(echo "$pr_info" | jq -r '.[0].number // empty')
  pr_title=$(echo "$pr_info" | jq -r '.[0].title // empty')
  pr_base_branch=$(echo "$pr_info" | jq -r '.[0].base.ref // "main"')

  commit_date=$(echo "$commit_info" | jq -r '.commit.author.date')
  commit_date_fmt=$(date -u -d "$commit_date" +"%Y-%m-%d %H:%M UTC")
  commit_sha_short=$(echo "$GITHUB_SHA" | cut -c1-7)

  echo "## $TITLE"$'\n'
  if [[ -n "$pr_url" ]]; then
    echo "**Source:** [#$pr_number - $pr_title]($pr_url)"
  fi
  echo "**Date:** $commit_date_fmt"
  echo "**Commit:** \`$commit_sha_short\` ($pr_base_branch)"
  echo
}

function build_table_for_prefix() {
  local prefix="$1"
  local title sample_file header

  title=$(format_title "$prefix")
  echo "### $title"$'\n'

  sample_file=$(find "$REPORT_DIR" -type f -name "${prefix}_*.txt" | head -n1)
  header=$(head -n 2 "$sample_file")
  echo "$header"

  find "$REPORT_DIR" -type f -name "${prefix}_*.txt" | sort \
    | while read -r f; do
        tail -n +3 "$f"
      done | grep -v '^$'
  echo
}

function generate_message() {
  local message
  message="$(build_metadata_section)"
  for prefix in $(get_prefixes); do
    message+=$(build_table_for_prefix "$prefix")
  done
  echo "$message"
}

function save_message_as_json() {
  local msg="$1"
  local escaped
  escaped=$(jq -Rs . <<< "$msg")
  echo "{\"text\": $escaped}" > mattermost.json
}

function main() {
  local message
  message=$(generate_message)
  echo "$message"
  save_message_as_json "$message"
}

main "$@"
