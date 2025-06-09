#!/bin/bash
set -euo pipefail

get_issue_labels() {
  curl -s -H "Authorization: token $GH_TOKEN" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER" |
    jq -r '.labels[].name'
}

add_labels_to_issue() {
  curl -s -X POST -H "Authorization: token $GH_TOKEN" \
    -H "Content-Type: application/json" \
    -d "$ISSUE_LABELS" \
    "$API_URL/repos/$REPO/issues/$ISSUE_NUMBER/labels" > /dev/null
}

build_project_card() {
  local PROJECT_ID="$1"
  local TITLE="$2"
  local BODY="Card automatically created from issue #$ISSUE_NUMBER in repository $REPO."
  jq -n \
    --arg projectId "$PROJECT_ID" \
    --arg title "$TITLE" \
    --arg body "$BODY" \
    '{
      query: "mutation($projectId: ID!, $title: String!, $body: String!) {
        addProjectV2ItemByTitle(input: {
          projectId: $projectId
          title: $title
          body: $body
        }) {
          item {
            id
          }
        }
      }",
      variables: {
        projectId: $projectId,
        title: $title,
        body: $body
      }
    }'
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

create_issue_card() {
  CARD_RESPONSE=$(curl -s -H "Authorization: bearer $GH_TOKEN" -X POST -d "$(build_project_card "$PROJECT_ID" "$ISSUE_NAME")" "$API_URL/graphql")
  echo "$CARD_RESPONSE" | jq -r '.data.addProjectV2ItemByTitle.item.id'

  if [ -z "$CARD_RESPONSE" ]; then
    echo "Failed to create project card for issue #$ISSUE_NUMBER"
    exit 1
  fi


  add_labels_to_issue
}

main() {
  echo "Checking labels for issue #$ISSUE_NUMBER..."

  if echo "$(get_issue_labels)" | grep -q "$LABEL_NAME"; then
    echo "Label '$LABEL_NAME' found."

    ITEM_ID=$(create_issue_card)
    echo "Project card ID: $ITEM_ID"

    echo "🔍 Fetching project columns..."
    FIELDS_RESPONSE=$(curl -s -H "Authorization: bearer $GH_TOKEN" -X POST -d "$(get_project_fields)" "$API_URL/graphql")

    PROJECT_ID=$(echo "$FIELDS_RESPONSE" | jq -r '.data.organization.projectV2.id')
    STATUS_FIELD=$(echo "$FIELDS_RESPONSE" | jq -c '.data.organization.projectV2.fields.nodes[] | select(.name == "Status")')
    STATUS_FIELD_ID=$(echo "$STATUS_FIELD" | jq -r '.id')
    TARGET_COLUMN_ID=$(echo "$STATUS_FIELD" | jq -r --arg col "$TARGET_COLUMN_NAME" '.options[] | select(.name == $col) | .id')

    echo "Project ID: $PROJECT_ID"
    echo "Field 'Status': $STATUS_FIELD_ID"
    echo "Target column: $TARGET_COLUMN_NAME ($TARGET_COLUMN_ID)"

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

main
