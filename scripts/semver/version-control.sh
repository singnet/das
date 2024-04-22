#!/bin/bash
set -e

_section() {
    Message="${@}"
    echo "_____________________________________________________________________________"
    echo "${Message^^}"
}

_info() {
    echo -e "[INFO] $@"
}

_error() {
    echo "[ERROR] $@" >&2
    exit 1
}

_test() {
    if [ "$1" = "_test" ]; then
        _section "Testing function"
        Command="${1}"
        ExpectedResult="${2}"
        _var Command
        _var ExpectedResult
        ActualResult=$(eval "$Command")
        _var ActualResult
        if [ "$ActualResult" != "$ExpectedResult" ]; then _error "Test failed"; else _info "Test passed"; fi
    fi
}

_var() {
    VariableName="${1}"
    VariableValue="${!VariableName}"
    _info "${VariableName}: ${VariableValue}"
    echo "::set-output name=${VariableName}::${VariableValue}"
}

CurrentFunctionName="$1"
_expose() {
    FunctionName="$1"
    if [ "$FunctionName" = "$CurrentFunctionName" ]; then
        _var FunctionName
        eval "$FunctionName"
    fi
}

_get_version_without_prefix() {
    VersionWithPrefix="${1}"
    Prefix="${2}"
    VersionWithoutPrefix="${VersionWithPrefix#${Prefix}}"
    echo "${VersionWithoutPrefix}"
}
_test "_get_version_without_prefix v7.2.0 v" "7.2.0"
_test "_get_version_without_prefix vx12.8.1 vx" "12.8.1"
_test "_get_version_without_prefix foo34.6.123 foo" "34.6.123"

_get_version_number() {
    Part="${1}"
    VersionWithPrefix="${2}"
    Prefix="${3}"

    VersionWithoutPrefix="$(_get_version_without_prefix ${VersionWithPrefix} ${Prefix})"

    major="${VersionWithoutPrefix%%.*}"
    minor="${VersionWithoutPrefix#*.}"
    minor="${minor%%.*}"
    patch="${VersionWithoutPrefix##*.}"

    if [ "$Part" = "major" ]; then echo "${major}"; fi
    if [ "$Part" = "minor" ]; then echo "${minor}"; fi
    if [ "$Part" = "patch" ]; then echo "${patch}"; fi
}
_test "_get_version_number major foo34.6.123 foo" "34"
_test "_get_version_number minor bar-3.233.23 bar-" "233"
_test "_get_version_number patch qux-v903.67.9 qux-v" "9"

_get_major_number() {
    Prefix="${1}"
    VersionWithPrefix="${2}"
    _get_version_number "major" "${VersionWithPrefix}" "${Prefix}"
}
_test "_get_major_number v v1.2.3" "1"

_get_minor_number() {
    Prefix="${1}"
    VersionWithPrefix="${2}"
    _get_version_number "minor" "${VersionWithPrefix}" "${Prefix}"
}
_test "_get_minor_number v v1.2.3" "2"

_get_patch_number() {
    Prefix="${1}"
    VersionWithPrefix="${2}"
    _get_version_number "patch" "${VersionWithPrefix}" "${Prefix}"
}
_test "_get_patch_number v v1.2.3" "3"

_init() {
    _section "Github Pipeline Template for Semantic Versioning"
    _var GITHUB_SERVER
    _var GITHUB_REPOSITORY
    _var GITHUB_REF_NAME
    _var GITHUB_SHA
    _var GITHUB_WORKFLOW
    _var VERSION_TAG_REGEX_PATTERN
    _var VERSION_STRATEGY
    _var VERSION_REQUIRE_CONFIRMATION
    _var GITHUB_ACTOR
    _var GITHUB_REPOSITORY_OWNER
    _var CHANGELOG_PATH

    _section "Cloning repository"
    git clone "https://${GITHUB_ACTOR}:${GITHUB_TOKEN}@${GITHUB_SERVER}/${GITHUB_REPOSITORY}.git" /clone
    cd /clone

    git checkout "${MAIN_BRANCH}"

    git fetch --tags

    # _info "Connecting to github cli"

    # echo $GITHUB_TOKEN | gh auth login --with-token

    if [ "$GITHUB_REF_NAME" = "" ]; then
        _error "This pipeline is not running on a branch"
    fi

    # if [ "$GITHUB_REF_NAME" != "$(git rev-parse --abbrev-ref HEAD)" ]; then
    #     _error "This pipeline is not running on the main branch"
    # fi

    # if [ "${GITHUB_SHA}" != "$(git log --pretty=format:'%H' -n 1)" ]; then
    #     _error "This pipeline is not running on the main branch last commit"
    # fi
}

_add_write_repository_permission() {
    _section "Adding permission to push tags in repository"

    _var GITHUB_USER_NAME
    _var GITHUB_USER_EMAIL
    if [ "${GITHUB_USER_NAME}" = "" ] ||
        [ "${GITHUB_USER_EMAIL}" = "" ] ||
        [ "${GITHUB_TOKEN}" = "" ]; then

        _info "Check that the following variables are set:"
        _info "- GITHUB_USER_NAME"
        _info "- GITHUB_USER_EMAIL"
        _info "- GITHUB_TOKEN"
        _error "Missing write repository permission"
    fi

    git config user.name "${GITHUB_USER_NAME}"
    git config user.email "${GITHUB_USER_EMAIL}"
    Url="https://${GITHUB_ACTOR}:${GITHUB_TOKEN}@${GITHUB_SERVER}/${GITHUB_REPOSITORY}.git"
    git remote set-url origin "${Url}"
}

_add_or_move_tag() {
    Tag="${1}"
    Message="${2}"

    if git tag --list | grep -q "^$Tag$"; then
        _info "Deleting tag $Tag"
        git tag -d "${Tag}"
        git push --delete origin "${Tag}"
    fi

    _info "Pushing tag $Tag"
    git tag -a "${Tag}" -m "${Message}"
    git push origin "${Tag}"
}

_delete_tag() {
    Tag="${1}"
    _info "Deleting tag $Tag"
    git tag -d "${Tag}"
    git push --delete origin "${Tag}"
}

_bump_release() {
    _section "Bumping release"

    local ScapedReleaseNote="$(printf '%s' "${ReleaseNote}" | jq -sRr '@json' | sed 's/\\\\n/\\n/g')"

    local JsonPayload='{"tag_name": "'"$NextPatchTag"'", "target_commitish": "'"$MAIN_BRANCH"'", "name": "'"$ReleaseName"'", "draft": false, "prerelease": false, "body": '$ScapedReleaseNote', "generate_release_notes": false}'

    Msg="Bump release: ${ReleaseName}"
    _info "${Msg}"

    # _info "${JsonPayload}"
    echo "${JsonPayload}"

    _info "https://api.github.com/repos/${GITHUB_REPOSITORY}/releases"

    curl -X POST "https://api.github.com/repos/${GITHUB_REPOSITORY}/releases" \
        -H "Authorization: token ${GITHUB_TOKEN}" \
        -H "Accept: application/vnd.github+json" \
        -H "X-GitHub-Api-Version: 2022-11-28" \
        -d "${JsonPayload}"
}

_determine_release_note() {
    if [ -e "${CHANGELOG_PATH}" ]; then
        ReleaseNote=$(awk -v ORS='\n' '1' "${CHANGELOG_PATH}")
        ReleaseNote=$(printf '%s' "${ReleaseNote}" | jq -sRr '@json' | sed 's/\\\\n/\\n/g')
    else
        ReleaseNote=""
    fi

    _var ReleaseNote
}

function _check_for_uncommitted_changes() {
    if git diff --quiet && git diff --staged --quiet; then
        echo "Skipping commit changes because no files were changed to be committed"
        return 1
    fi
    return 0
}

_update_changelog() {
    if [ $NextMajorNumber -gt $PreviousMajorNumber ] || [ $NextMinorNumber -gt $PreviousMinorNumber ]; then
        _info "Reseting changelog file"
        _info "${CHANGELOG_PATH}"

        echo "" >"$CHANGELOG_PATH"

        if _check_for_uncommitted_changes; then
            git add "$CHANGELOG_PATH"
            git commit -m "Reset CHANGELOG"
            git push origin "${MAIN_BRANCH}"
        fi

    else
        _info "Skipping reset changelog file"
    fi
}

_resolve_release() {
    ReleaseName="Release ${NextPatchTag}"

    _determine_release_note
    _update_changelog

    _var ReleaseName
}

_bump_version() {
    _section "Bumping version"

    if ((NextMajorNumber < PreviousMajorNumber)); then _error "The next version cannot be less than the previous version"; fi
    if ((NextMajorNumber == PreviousMajorNumber)); then
        if ((NextMinorNumber < PreviousMinorNumber)); then _error "The next version cannot be less than the previous version"; fi
        if ((NextMinorNumber == PreviousMinorNumber)); then
            if ((NextPatchNumber < PreviousPatchNumber)); then _error "The next version cannot be less than the previous version"; fi
            if ((NextPatchNumber == PreviousPatchNumber)); then _error "The next version cannot be the same as the previous version"; fi
        fi
    fi

    Msg="Bump version: ${PreviousPatchTag} -> ${NextPatchTag}"
    _info "$Msg"
    _add_or_move_tag ${NextPatchTag} "$Msg"

    if [ "${NextMinorTag}" = "${PreviousMinorTag}" ]; then
        Msg="Move minor version tag: $NextMinorTag"
        # TODO from PreviousMinorVersionCommit -> to NextMinorVersionCommit
    else
        Msg="Bump minor version: ${PreviousMinorTag} -> ${NextMinorTag}"
    fi
    _info "$Msg"
    _add_or_move_tag ${NextMinorTag} "${Msg}"

    if [ "${NextMajorTag}" = "${PreviousMajorTag}" ]; then
        Msg="Move major version tag: $NextMajorTag"
        # TODO from PreviousMajorVersionCommit -> to NextMajorVersionCommit
    else
        Msg="Bump major version: ${PreviousMajorTag} -> ${NextMajorTag}"
    fi
    _info "$Msg"
    _add_or_move_tag ${NextMajorTag} "${Msg}"
}

_resolve_version_tag_prefix() {
    _section "Resolving version tag prefix"
    _var VERSION_TAG_REGEX_PATTERN

    # TODO validate
    # if [[ $VERSION_TAG_REGEX_PATTERN != /^* ]]; then
    #     _error "Invalid VERSION_TAG_REGEX_PATTERN"
    # fi
    # if [[ $VERSION_TAG_REGEX_PATTERN != *\d+\.\d+\.\d+$/ ]]; then
    #     _error "Invalid VERSION_TAG_REGEX_PATTERN"
    # fi

    without_prefix="${VERSION_TAG_REGEX_PATTERN#/^}"
    VersionTagPrefix="${without_prefix%\\d+\\.\\d+\\.\\d+$/}"
    _var VersionTagPrefix
    # TODO validate VersionTagPrefix
}

_resolve_previous_version_vars() {
    _section "Resolving previous version"

    VersionTags=$(git for-each-ref --sort=-taggerdate --format '%(refname:short)' refs/tags | grep -E '^.*?[0-9]+\.[0-9]+\.[0-9]+$' | head -n 1)
    if [ "x${VersionTags}" = "x" ]; then
        PreviousVersionTag="${VersionTagPrefix}0.0.0"
    else
        PreviousVersionTag=$(echo $VersionTags | cut -f1 -d" ")
    fi

    PreviousMajorNumber="$(_get_major_number $VersionTagPrefix $PreviousVersionTag)"
    PreviousMinorNumber="$(_get_minor_number $VersionTagPrefix $PreviousVersionTag)"
    PreviousPatchNumber="$(_get_patch_number $VersionTagPrefix $PreviousVersionTag)"

    PreviousMajorTag="${VersionTagPrefix}${PreviousMajorNumber}"
    PreviousMinorTag="${VersionTagPrefix}${PreviousMajorNumber}.${PreviousMinorNumber}"
    PreviousPatchTag="${VersionTagPrefix}${PreviousMajorNumber}.${PreviousMinorNumber}.${PreviousPatchNumber}"

    _var PreviousVersionTag
    _var PreviousMajorNumber
    _var PreviousMinorNumber
    _var PreviousPatchNumber
    _var PreviousMajorTag
    _var PreviousMinorTag
    _var PreviousPatchTag
}

_resolve_next_version_vars_from_version() {
    _section "Resolving next version"

    NextVersionTag="${1}"
    # TODO validate version tag

    NextMajorNumber="$(_get_major_number $VersionTagPrefix $NextVersionTag)"
    NextMinorNumber="$(_get_minor_number $VersionTagPrefix $NextVersionTag)"
    NextPatchNumber="$(_get_patch_number $VersionTagPrefix $NextVersionTag)"

    NextMajorTag="${VersionTagPrefix}${NextMajorNumber}"
    NextMinorTag="${VersionTagPrefix}${NextMajorNumber}.${NextMinorNumber}"
    NextPatchTag="${VersionTagPrefix}${NextMajorNumber}.${NextMinorNumber}.${NextPatchNumber}"

    _var NextVersionTag
    _var NextMajorNumber
    _var NextMinorNumber
    _var NextPatchNumber
    _var NextMajorTag
    _var NextMinorTag
    _var NextPatchTag
}

_resolve_next_version_vars_from_change_level() {
    _section "Resolving next version from change level"

    ChangeLevel="${1}"

    NextMajorNumber=${PreviousMajorNumber}
    NextMinorNumber=${PreviousMinorNumber}
    NextPatchNumber=${PreviousPatchNumber}

    if [ "${ChangeLevel}" = "major" ]; then
        let "NextMajorNumber++" || true
        NextMinorNumber="0"
        NextPatchNumber="0"
    elif [ "${ChangeLevel}" = "minor" ]; then
        let "NextMinorNumber++" || true
        NextPatchNumber="0"
    else
        let "NextPatchNumber++" || true
    fi

    NextMajorTag="${VersionTagPrefix}${NextMajorNumber}"
    NextMinorTag="${VersionTagPrefix}${NextMajorNumber}.${NextMinorNumber}"
    NextPatchTag="${VersionTagPrefix}${NextMajorNumber}.${NextMinorNumber}.${NextPatchNumber}"

    NextVersionTag="${NextPatchTag}"

    _var NextVersionTag
    _var NextMajorNumber
    _var NextMinorNumber
    _var NextPatchNumber
    _var NextMajorTag
    _var NextMinorTag
    _var NextPatchTag
}

bump-version-from-change-level() {
    _init
    _var CHANGE_LEVEL
    _resolve_version_tag_prefix
    _resolve_previous_version_vars
    _resolve_next_version_vars_from_change_level "${CHANGE_LEVEL}"
    _add_write_repository_permission
    _bump_version
}
_expose bump-version-from-change-level

bump-version-from-variable-value() {
    _init
    _var VERSION_VARIABLE_NAME
    _resolve_version_tag_prefix
    _resolve_previous_version_vars
    _resolve_next_version_vars_from_version "${!VERSION_VARIABLE_NAME}"
    _add_write_repository_permission
    _resolve_release
    _bump_version
    _bump_release
}
_expose bump-version-from-variable-value

bump-version-from-properties-file() {
    _init
    _var VERSION_FILE
    _var VERSION_FILE_PROPERTY
    _var VERSION_FILE_TYPE

    _error "Not implemented: $0"
}
# TODO _expose bump-version-from-properties-file

bump-version-from-commit-messages() {
    _init
    _error "Not implemented: $0"
}
# TODO _expose bump-version-from-commit-messages

remove-test-version-tags() {
    _init
    _resolve_version_tag_prefix
    _add_write_repository_permission
    TestVersionTags=$(git tag -l)
    for Tag in $TestVersionTags; do
        if [[ "$Tag" == "$VersionTagPrefix"* ]]; then
            _info "Preserve tag: ${Tag}"
        else
            _info "Delete tag: ${Tag}"
            _delete_tag "${Tag}"
        fi
    done
}
_expose remove-test-version-tags
