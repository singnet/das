#!/bin/bash

set -e

# PATHS
workdir=$(pwd)
release_notes_file_path="$workdir/docs/release-notes.md"
package_order=("das-toolbox" "hyperon-das" "hyperon-das-atomdb" "das-serverless-functions" "das-metta-parser")

function get_repository_changelog() {
    local changelog_path="$1"

    if verify_file_exists "$changelog_path"; then
        cat "$changelog_path"
    fi
}

function create_release_notes_file() {
    local release_notes_title="$1"
    print ":yellow:Creating document at $release_notes_file_path...:/yellow:"
    mkdir -p "$(dirname "$release_notes_file_path")"
    echo -e "$release_notes_title\n\n" >"$release_notes_file_path"
}

function map_package_name() {
  case "$1" in
    das-toolbox) echo "Toolbox das-cli" ;;
    das-serverless-functions) echo "FaaS functions" ;;
    das-metta-parser) echo "MeTTa Parser" ;;
    *) echo "$1" ;;
  esac
}

function sort_packages() {
    local sorted_packages=()
    local unsorted_packages=($(echo "$packages_pending_update" | jq -c '.[]'))

    for i in "${!package_order[@]}"; do
        for j in "${!unsorted_packages[@]}"; do 
            IFS='|' read -r package_name _ <<<"$(extract_packages_pending_update_details "${unsorted_packages[j]}")"

            if [[ "${package_order[i]}" == "$package_name" ]]; then
                sorted_packages+=("${unsorted_packages[j]}")
                unset unsorted_packages[j]
            fi
        done
    done

    sorted_packages+=("${unsorted_packages[@]}")

    jq -c -n --argjson arr "$(printf '%s\n' "${sorted_packages[@]}" | jq -s .)" '$arr'
}

function generate_release_notes_block() {
    local new_block=""
    local sorted_packages_pending_update=$(sort_packages)


    for package in $(echo "$sorted_packages_pending_update" | jq -c '.[]'); do
        IFS='|' read -r package_name current_version new_version repository_path <<<"$(extract_packages_pending_update_details "$package")"

        local package_name_alias=$(map_package_name "$package_name")

        new_block+="* ${package_name_alias}: ${new_version}\n"
    done

    echo -e "## DAS Version $new_package_version\n\n$new_block"
}

function generate_release_notes_changelog_block() {
    local new_block=""
    local sorted_packages_pending_update=$(sort_packages)

    for package in $(echo "$sorted_packages_pending_update" | jq -c '.[]'); do
        IFS='|' read -r package_name current_version new_version repository_path repository_name repository_owner repository_workflow repository_ref <<<"$(extract_packages_pending_update_details "$package")"

        local changelog=$(get_repository_changelog "$repository_path/CHANGELOG" | sed '/^\s*$/d' | awk 'NF')
        local package_name_alias=$(map_package_name "$package_name")

        new_block+="#### $package_name_alias $new_version\n\n\`\`\`\n${changelog:-"No changelog available"}\n\`\`\`\n\n"
    done

    echo -e "\n\n### Changelog\n\n$new_block\n"
}

function generate_release_notes() {
    local release_notes_title="# Release Notes"
    local file_name=$(basename "$release_notes_file_path")

    if boolean_prompt "Would you like to update $file_name? [yes/no] "; then
        if ! verify_file_exists "$release_notes_file_path"; then
            create_release_notes_file "$release_notes_title"
        fi

        local new_release_notes=$(generate_release_notes_block)
        new_release_notes+=$(generate_release_notes_changelog_block)

        local title_line_number=$(find_line_number_by_text_in_file "$release_notes_file_path" "$release_notes_title")

        append_content_in_file "$release_notes_file_path" "$new_release_notes" "$title_line_number"
        print ":green:New release note section added to the file: $release_notes_file_path:/green:"
    else
        print ":yellow:No changes made to the release notes document.:/yellow:"
    fi
}

function change_bazel_das_version_definition() {
    local bazelrc_path="src/.bazelrc"

    if [ ! -f "$bazelrc_path" ]; then
        echo "File $bazelrc_path could not be found"
        exit 1
    fi

    sed -i.bak -E "s|(--define=DAS_VERSION=).*|\1${new_package_version}|" "$bazelrc_path"
    echo "DAS_VERSION updated to $new_package_version in $bazelrc_path"
}

change_bazel_das_version_definition
generate_release_notes
