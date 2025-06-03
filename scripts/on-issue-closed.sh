#!/bin/bash
set -euo pipefail

get_issue_labels() {
  curl -s -H "Authorization: token $GH_TOKEN" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER" |
    jq -r '.labels[].name'
}

remove_label_from_issue() {
  curl -s -X DELETE -H "Authorization: token $GH_TOKEN" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER/labels/$(urlencode "$LABEL_NAME")"
}

reopen_issue() {
  curl -s -X PATCH -H "Authorization: token $GH_TOKEN" \
    -H "Content-Type: application/json" \
    -d '{"state": "open"}' \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER"
}

urlencode() {
  local str="$1"
  jq -nr --arg v "$str" '$v|@uri'
}

get_project_fields() {
  jq -n --arg org "$OWNER" --argjson number "$PROJECT_NUMBER" '{
    query: "query ($org: String!, $number: Int!) {
      organization(login: $org) {
        projectV2(number: $number) {
          id
          fields(first: 50) {
            nodes {
              ... on ProjectV2SingleSelectField {
                id
                name
                options { id name }
              }
            }
          }
        }
      }
    }",
    variables: { org: $org, number: $number }
  }'
}

build_get_project_item_query() {
  jq -n --arg owner "$OWNER" --arg name "$(basename "$REPO")" --argjson issue "$ISSUE_NUMBER" '{
    query: "query($owner: String!, $name: String!, $issue: Int!) {
      repository(owner: $owner, name: $name) {
        issue(number: $issue) {
          projectItems(first: 10) {
            nodes {
              id
              project { id title }
            }
          }
        }
      }
    }",
    variables: {
      owner: $owner,
      name: $name,
      issue: $issue
    }
  }'
}

get_project_item_id_for_issue() {
  local query
  query=$(build_get_project_item_query)

  curl -s -H "Authorization: bearer $GH_TOKEN" -H "Content-Type: application/json" \
    -d "$query" "$API_URL/graphql" | jq -r '.data.repository.issue.projectItems.nodes[0].id'
}

build_mutation_payload() {
  local PROJECT_ID="$1"
  local itemId="$2"
  local fieldId="$3"
  local optionId="$4"

  jq -n --arg projectId "$PROJECT_ID" \
    --arg itemId "$itemId" \
    --arg fieldId "$fieldId" \
    --arg optionId "$optionId" \
    '{
    query: "mutation ($projectId: ID!, $itemId: ID!, $fieldId: ID!, $optionId: String!) {
      updateProjectV2ItemFieldValue(input: {
        projectId: $projectId,
        itemId: $itemId,
        fieldId: $fieldId,
        value: { singleSelectOptionId: $optionId }
      }) {
        projectV2Item { id }
      }
    }",
    variables: {
      projectId: $projectId,
      itemId: $itemId,
      fieldId: $fieldId,
      optionId: $optionId
    }
  }'
}

move_issue_to_column() {
  echo "Checking labels for issue #$ISSUE_NUMBER..."

  if echo "$(get_issue_labels)" | grep -q "$LABEL_NAME"; then
    echo "Label '$LABEL_NAME' found."

    remove_label_from_issue
    echo "Label '$LABEL_NAME' removed."

    reopen_issue
    echo "Issue reopened."

    sleep 10s
    echo "🔍 Fetching project columns..."
    FIELDS_RESPONSE=$(curl -s -H "Authorization: bearer $GH_TOKEN" -X POST -d "$(get_project_fields)" "$API_URL/graphql")


    PROJECT_ID=$(echo "$FIELDS_RESPONSE" | jq -r '.data.organization.projectV2.id')
    STATUS_FIELD=$(echo "$FIELDS_RESPONSE" | jq -c '.data.organization.projectV2.fields.nodes[] | select(.name == "Status")')
    STATUS_FIELD_ID=$(echo "$STATUS_FIELD" | jq -r '.id')
    TARGET_COLUMN_ID=$(echo "$STATUS_FIELD" | jq -r --arg col "$TARGET_COLUMN_NAME" '.options[] | select(.name == $col) | .id')

    echo "Project ID: $PROJECT_ID"
    echo "Field 'Status': $STATUS_FIELD_ID"
    echo "Target column: $TARGET_COLUMN_NAME ($TARGET_COLUMN_ID)"

    ITEM_ID=$(get_project_item_id_for_issue)
    echo "Project card ID: $ITEM_ID"

    echo "Moving card to '$TARGET_COLUMN_NAME'..."
    curl -s -X POST "$API_URL/graphql" \
      -H "Authorization: bearer $GH_TOKEN" \
      -H "Content-Type: application/json" \
      -d "$(build_mutation_payload "$PROJECT_ID" "$ITEM_ID" "$STATUS_FIELD_ID" "$TARGET_COLUMN_ID")"

    echo "Card moved successfully!"
  else
    echo "Label '$LABEL_NAME' not found on issue #$ISSUE_NUMBER. No action taken."
  fi
}

move_issue_to_column
