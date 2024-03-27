#!/bin/bash

set -e

release_notes_file_path="$(pwd)/docs/release-notes.md"

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

function generate_release_notes_block() {
    local new_block=""

    for package in $(echo "$packages_pending_update" | jq -c '.[]'); do
        IFS='|' read -r package_name current_version new_version repository_path <<<"$(extract_packages_pending_update_details "$package")"

        new_block+="* $package_name: $new_version\n"
    done

    echo -e "## DAS Version $new_package_version\n\n$new_block"
}

function generate_release_notes_changelog_block() {
    local new_block=""

    for package in $(echo "$packages_pending_update" | jq -c '.[]'); do
        IFS='|' read -r package_name current_version new_version repository_path repository_name repository_owner repository_workflow repository_ref <<<"$(extract_packages_pending_update_details "$package")"

        local changelog=$(get_repository_changelog "$repository_path/CHANGELOG")

        new_block+="#### $package_name $new_version\n\n\`\`\`\n$changelog\n\`\`\`\n\n"
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

generate_release_notes
