#!/bin/bash
set -euo pipefail

get_issue_labels() {
  curl -s -H "Authorization: token $GH_TOKEN" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER" |
    jq -r '.labels[].name'
}

add_labels_to_issue() {
  local ISSUE_NUMBER="$1"

  curl -s -X POST -H "Authorization: token $GH_TOKEN" \
    -H "Content-Type: application/json" \
    -d "$ISSUE_LABELS" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER/labels" >/dev/null
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

add_issue_to_project() {
  local PROJECT_ID="$1"
  local ISSUE_NODE_ID="$2"

  curl -s -X POST \
    -H "Authorization: Bearer $GH_TOKEN" \
    -H "Content-Type: application/json" \
    "$API_URL/graphql" \
    -d "$(jq -n \
      --arg projectId "$PROJECT_ID" \
      --arg contentId "$ISSUE_NODE_ID" \
      '{
        query: "mutation($projectId: ID!, $contentId: ID!) { addProjectV2ItemById(input: {projectId: $projectId, contentId: $contentId}) { item { id } } }",
        variables: { projectId: $projectId, contentId: $contentId }
      }')"
}

create_issue_card() {
  local PROJECT_ID="$1"
  local ASSIGNEES="$2"
  local TITLE="$ISSUE_NAME"
  local BODY="This issue was automatically created from issue #$ISSUE_NUMBER in the repository $REPO."

  if [[ -n "$ASSIGNEES" ]]; then
    ASSIGNEES_JSON=$(jq -n --argjson arr "$(jq -R 'split(",")' <<<"$ASSIGNEES")" '$arr')
  else
    ASSIGNEES_JSON="[]"
  fi

  local CARD_RESPONSE=$(curl -s -X POST \
    -H "Authorization: Bearer $GH_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    "$API_URL/repos/$REPO/issues" \
    -d "{\"title\": \"$TITLE\", \"body\": \"$BODY\", \"assignees\": $ASSIGNEES_JSON }")

  local ITEM_ID=$(echo "$CARD_RESPONSE" | jq -r '.id')

  if [ -z "$ITEM_ID" ]; then
    echo "Failed to create project card for issue #$ISSUE_NUMBER"
    return 1
  fi

  echo "$CARD_RESPONSE"
}

get_issue_assignees() {
  local ISSUE_NUMBER="$1"

  curl -s -H "Authorization: Bearer $GH_TOKEN" \
    -H "Accept: application/vnd.github+json" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER" | jq -r '.assignees | map(.login) | join(",")'
}

move_issue_to_column() {
  local PROJECT_ID="$1"
  local PROJECT_ITEM_ID="$2"
  local FIELD_ID="$3"
  local TARGET_COLUMN_ID="$4"

  curl -s -X POST "$API_URL/graphql" \
    -H "Authorization: bearer $GH_TOKEN" \
    -H "Content-Type: application/json" \
    -d "$(build_mutation_payload "$PROJECT_ID" "$PROJECT_ITEM_ID" "$FIELD_ID" "$TARGET_COLUMN_ID")"
}

main() {
  echo "Checking labels for issue #$ISSUE_NUMBER..."

  if echo "$(get_issue_labels)" | grep -q "$LABEL_NAME"; then
    echo "Label '$LABEL_NAME' found."

    echo "🔍 Fetching project columns..."
    FIELDS_RESPONSE=$(curl -s -H "Authorization: bearer $GH_TOKEN" -X POST -d "$(get_project_fields)" "$API_URL/graphql")

    PROJECT_ID=$(echo "$FIELDS_RESPONSE" | jq -r '.data.organization.projectV2.id')
    STATUS_FIELD=$(echo "$FIELDS_RESPONSE" | jq -c '.data.organization.projectV2.fields.nodes[] | select(.name == "Status")')
    PRIORITY_FIELD=$(echo "$FIELDS_RESPONSE" | jq -c '.data.organization.projectV2.fields.nodes[] | select(.name == "Priority")')

    STATUS_FIELD_ID=$(echo "$STATUS_FIELD" | jq -r '.id')
    PRIORITY_FIELD_ID=$(echo "$HIGH_FIELD" | jq -r '.id')

    TARGET_COLUMN_ID=$(echo "$STATUS_FIELD" | jq -r --arg col "$TARGET_COLUMN_NAME" '.options[] | select(.name == $col) | .id')

    PRIORITY_COLUMN_NAME="High"
    PRIORITY_COLUMN_ID=$(echo "$PRIORITY_FIELD" | jq -r --arg col "$PRIORITY_COLUMN_NAME" '.options[] | select(.name == $col) | .id')

    echo "Project ID: $PROJECT_ID"
    echo "Field 'Status': $STATUS_FIELD_ID"
    echo "Target column: $TARGET_COLUMN_NAME ($TARGET_COLUMN_ID)"
    echo "Priority field: $PRIORITY_FIELD_ID"

    if [[ "$ISSUE_ASSIGNEES" == "[]" || -z "$ISSUE_ASSIGNEES" ]]; then
      ISSUE_ASSIGNEES=$(get_issue_assignees "$ISSUE_NUMBER")
      if [ -z "$ISSUE_ASSIGNEES" ]; then
        echo "No assignees found for issue #$ISSUE_NUMBER."
      fi
    fi
    echo "Assignees for the new project card: $ISSUE_ASSIGNEES"

    ITEM_RESPONSE=$(create_issue_card "$PROJECT_ID" "$ISSUE_ASSIGNEES")SSUE_ASSIGNEES=$(get_issue_assignees "$ISSUE_NUMBER")
    if [ -z "$ISSUE_ASSIGNEES" ]; then
      echo "No assignees found for issue #$ISSUE_NUMBER."
    fi
    echo "Assignees for the new project card: $ISSUE_ASSIGNEES"

    ITEM_RESPONSE=$(create_issue_card "$PROJECT_ID" "$ISSUE_ASSIGNEES")

    local ITEM_ID=$(echo "$ITEM_RESPONSE" | jq -r '.id')
    local ITEM_NUMBER=$(echo "$ITEM_RESPONSE" | jq -r '.number')
    local ITEM_NODE_ID=$(echo "$ITEM_RESPONSE" | jq -r '.node_id')

    if [ -z "$ITEM_ID" ]; then
      echo "Failed to create project card for issue #$ISSUE_NUMBER"
      exit 1
    fi
    echo "Project card ID: $ITEM_ID"

    add_labels_to_issue "$ITEM_NUMBER"
    echo "Labels added to issue #$ITEM_NUMBER."

    PROJECT_ITEM_RESPONSE=$(add_issue_to_project "$PROJECT_ID" "$ITEM_NODE_ID")

    local PROJECT_ITEM_ID=$(echo "$PROJECT_ITEM_RESPONSE" | jq -r '.data.addProjectV2ItemById.item.id')

    echo "$PROJECT_ITEM_RESPONSE"

    if [[ -z "$PROJECT_ITEM_ID" || "$PROJECT_ITEM_ID" == "null" ]]; then
      echo "Failed to add issue to project"
      echo "$PROJECT_ITEM_RESPONSE"
      return 1
    fi

    echo "Issue #$ITEM_NUMBER added to project $PROJECT_NUMBER."

    echo "Move card to $PRIORITY_COLUMN_NAME column..."
    move_issue_to_column "$PROJECT_ID" "$PROJECT_ITEM_ID" "$PRIORITY_FIELD_ID" "$PRIORITY_COLUMN_ID"
    echo "Card moved to $PRIORITY_COLUMN_NAME column successfully!"

    echo "Moving card to '$TARGET_COLUMN_NAME'..."
    move_issue_to_column "$PROJECT_ID" "$PROJECT_ITEM_ID" "$STATUS_FIELD_ID" "$TARGET_COLUMN_ID"
    echo "Card moved successfully!"
  else
    echo "Label '$LABEL_NAME' not found on issue #$ISSUE_NUMBER. No action taken."
  fi
}

main
