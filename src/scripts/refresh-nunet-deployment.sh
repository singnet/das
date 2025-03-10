#!/bin/bash

if [ -z "$1" ]; then
    echo "Error: No ensemble name provided."
    echo "Usage: $0 <ensemble_name>"
    exit 1
fi

ensemble_name="$1"

deployments_json=$(nunet actor cmd --context user /dms/node/deployment/list)

running_id=$(echo "$deployments_json" | jq -r '.Deployments | to_entries[] | select(.value == "Running") | .key')

echo "Checking for running deployments..."
if [ -n "$running_id" ]; then
    echo "Found running deployment: $running_id. Shutting down..."
    nunet actor cmd --context user /dms/node/deployment/shutdown -i "$running_id"
else
    echo "No running deployment found."
fi

echo "Updating Docker images..."
images=(
    "trueagi/das:attention-broker-poc"
    "trueagi/das:query-agent-poc"
    "trueagi/das:link-creation-agent-poc"
    "trueagi/das:link-creation-client-poc"
)

for image in "${images[@]}"; do
    echo "Pulling $image..."
    docker pull "$image"
done

echo "Docker images updated."

echo "Creating new deployment with ensemble: $ensemble_name..."
nunet actor cmd --context user /dms/node/deployment/new -f /tmp/"$ensemble_name".yaml -t 300s

echo "Process completed."
