#!/bin/bash

set -e

function check_ubuntu() {
  if ! [ -f /etc/lsb-release ] || ! grep -q "Ubuntu" /etc/lsb-release; then
    echo "This script is only available for Ubuntu at the moment."
    exit 1
  fi
}

function detect_architecture() {
  ARCH=$(uname -m)
  if [[ "$ARCH" == "x86_64" ]]; then
    PACKAGE_ARCH="amd64"
  elif [[ "$ARCH" == "aarch64" ]]; then
    PACKAGE_ARCH="arm64"
  else
    echo "Unsupported architecture: $ARCH"
    exit 1
  fi
  echo "Detected architecture: $PACKAGE_ARCH"
}

function fetch_latest_release() {
  GITLAB_API_URL="https://gitlab.com/api/v4/projects/nunet%2Fdevice-management-service/releases"
  echo "Fetching the latest release of nunet/device-management-service..."
  response=$(curl -s $GITLAB_API_URL)
  latest_release=$(echo "$response" | jq -r '.[0].name')
  package_url=$(echo "$response" | jq -r --arg arch "$PACKAGE_ARCH" '.[0].assets.links[] | select(.name | test(".*" + $arch + ".deb$")) | .url')

  if [ -z "$package_url" ]; then
    echo "No $PACKAGE_ARCH.deb package found in the latest release."
    exit 1
  fi

  echo "Latest release: $latest_release"
  echo "Package URL: $package_url"
}

function download_package() {
  echo "Downloading package..."
  curl -L -o /tmp/nunet-dms.deb "$package_url"
}

function install_package() {
  echo "Installing the downloaded package..."
  sudo -S apt install -y /tmp/nunet-dms.deb
}

function cleanup() {
  echo "Cleaning up..."
  [ -f /tmp/nunet-dms.deb ] && rm /tmp/nunet-dms.deb
  [ -f /tmp/firecracker.tgz ] && rm /tmp/firecracker.tgz
  [ -f "$ENSEMBLE_FILE" ] && rm -f "$ENSEMBLE_FILE"
}

function check_firecracker_installed() {
  if command -v firecracker &> /dev/null; then
    echo "Firecracker is already installed."
    return 0 
  fi
  return 1
}

function check_nunet_installed() {
  if command -v nunet &> /dev/null; then
    echo "Nunet is already installed."
    return 0
  fi
  return 1
}

function fetch_firecracker_release() {
  echo "Fetching the latest release of Firecracker from GitHub..."
  
  FIRECRACKER_API_URL="https://api.github.com/repos/firecracker-microvm/firecracker/releases/latest"
  response=$(curl -s $FIRECRACKER_API_URL)
  
  asset_url=$(echo "$response" | jq -r '.assets[] | select(.name | test(".*x86_64.tgz$")) | .browser_download_url')

  if [ -z "$asset_url" ]; then
    echo "No valid tarball found for Firecracker."
    exit 1
  fi

  echo "Latest release URL: $asset_url"
}

function download_and_extract_firecracker() {
  echo "Downloading Firecracker..."
  curl -L -o /tmp/firecracker.tgz "$asset_url"
  
  echo "Extracting Firecracker..."
  
  tar -xvzf /tmp/firecracker.tgz -C /tmp

  EXTRACTED_FOLDER=$(basename "$(find /tmp -maxdepth 1 -type d -name "release-*" | head -1)")

  if [ -z "$EXTRACTED_FOLDER" ]; then
    echo "Error: Extracted folder not found."
    exit 1
  fi

  echo "Extracted folder: $EXTRACTED_FOLDER"

  FIRECRACKER_BIN=$(basename "$(find "/tmp/$EXTRACTED_FOLDER" -type f -name "firecracker*" -executable | head -1)")

  if [ -z "$FIRECRACKER_BIN" ]; then
    echo "Error: Firecracker binary not found in /tmp/$EXTRACTED_FOLDER."
    exit 1
  fi

  echo "Found Firecracker binary: $FIRECRACKER_BIN"

  echo "Moving Firecracker to /opt/firecracker..."
  sudo mv "/tmp/$EXTRACTED_FOLDER" /opt/firecracker

  echo "Creating a symlink to /usr/local/bin..."
  sudo ln -sf "/opt/firecracker/$FIRECRACKER_BIN" /usr/local/bin/firecracker
}

function check_and_remove_nunet_data() {
  echo "Checking if directory exists..."
  if [ -d "$HOME/.nunet" ]; then
    echo "Removing previous data..."
    rm -rf "$HOME/.nunet"
  else
    echo "No data directory found to remove."
  fi
}

function set_passphrase() {
  if [[ -z "$DMS_PASSPHRASE" ]]; then
    echo && echo "Creating a default passphrase. This will be used to encrypt all your identities."
      
    export DMS_PASSPHRASE="singularity"
    echo "Passphrase set successfully."
  fi
}

function set_organization_and_identity_names() {
  read -rp "Organization name (default: org): " ORG_NAME
  ORG_NAME=${ORG_NAME:-org}

  read -rp "User name (default: user): " USER_NAME
  USER_NAME=${USER_NAME:-user}
  
  read -rp "Node name (default: dms): " DMS_NAME
  DMS_NAME=${DMS_NAME:-dms}
}

function setup_nunet_capabilities() {
  nunet cap new "$USER_NAME"
  USER_DID=$(nunet key did "$USER_NAME")
  nunet cap new "$DMS_NAME"
  
  echo "Anchoring DMS context..."
  nunet cap anchor --context "$DMS_NAME" --root "$USER_DID"
  
  echo "DMS setup completed successfully!"
}

function create_stricted_network() {
  echo "Creating organization, user, and node..."


  if [ -f "$HOME/.nunet/cap/$ORG_NAME.cap" ]; then
    echo "'$ORG_NAME' already exists. Overwriting..."
    nunet cap new -f "$ORG_NAME"
  else
    nunet cap new "$ORG_NAME"
  fi

  ORG_DID=$(nunet key did "$ORG_NAME")
  GRANT_FROM_ORG=$(nunet cap grant --context "$ORG_NAME" --cap /dms/deployment --cap /public --cap /broadcast --topic /nunet --expiry "$EXPIRY_DATE" "$USER_DID")

  stricted_network_setup
}

function grant_network_permission() {
  echo "Granting permissions..."

  nunet cap grant --context "$ORG_NAME" --cap /dms/deployment --cap /public --cap /broadcast --topic /nunet --expiry "$EXPIRY_DATE" "$USER_DID" && echo
}

function join_network() {
  echo "Joining organization..."

  stricted_network_setup
}

function stricted_network_setup() {
  echo "Setting up nunet capabilities..."

  nunet cap anchor --context "$USER_NAME" --provide "$GRANT_FROM_ORG"

  echo "Creating public behaviors grant..."
  GRANT_FROM_USER=$(nunet cap grant --context "$USER_NAME" --cap /dms/deployment --cap /public --cap /broadcast --topic /nunet --expiry "$EXPIRY_DATE" "$ORG_DID")

  echo "Adding required anchor to DMS..."
  nunet cap anchor --context "$DMS_NAME" --require "$GRANT_FROM_USER"

  echo "Creating delegation to DMS..."
  DELEGATE_TOKEN=$(nunet cap delegate --context "$USER_NAME" --cap /dms/deployment --cap /public --cap /broadcast --topic /nunet --expiry "$EXPIRY_DATE" "$DMS_DID")

  echo "Adding provided anchor to DMS..."
  nunet cap anchor --context "$DMS_NAME" --provide "$DELEGATE_TOKEN"
  echo
}

function onboard_compute_resources() {
  while true; do
    read -rp "Enter the amount of RAM (GB): " RAM
    read -rp "Enter the number of CPU cores: " CPU
    read -rp "Enter the amount of Disk space (GB): " DISK

    OUTPUT=$(nunet actor cmd --context "$USER_NAME" /dms/node/onboarding/onboard --ram "$RAM" --cpu "$CPU" --disk "$DISK" --no-gpu | tail -n +2)
    SUCCESS=$(echo "$OUTPUT" | jq -r '.success')
    ERROR_MSG=$(echo "$OUTPUT" | jq -r '.error')

    if [[ "$SUCCESS" == "true" ]]; then
      echo "Onboarding completed successfully."
      break
    else
      echo "Onboarding error: $ERROR_MSG"
      echo "Please try again."
    fi
  done
}

function get_peer_id() {
if ! PEER_ID=$(nunet actor cmd /dms/node/peers/self -c "$USER_NAME" | jq -r '.id'); then
    echo "Error: Could not get the peer ID"
    exit 1
  fi

  echo "Peer ID: $PEER_ID"
}

function deploy_ensemble() {
  ENSEMBLE_FILE=$(mktemp /tmp/ensemble.XXXXXX.yaml)
  sed "s#\${PEER_ID}#$PEER_ID#g" src/assets/ensemble.yaml > "$ENSEMBLE_FILE"

  if ! nunet actor cmd --context user /dms/node/deployment/new -f "$ENSEMBLE_FILE" -t 300s; then
    echo "Deployment could not be done!" >&2
    exit 1
  fi

  echo "Deployment completed successfully."
}

function run_nunet_in_background() {
  echo "Starting nunet run in the background..."

  nohup nunet run > "$HOME/nunet_run.log" 2>&1 &

  sleep 30s

  echo "nunet run is now running in the background. Output is logged to $HOME/nunet_run.log"
}

function main() {
  check_ubuntu
  detect_architecture

  if ! check_nunet_installed; then
    fetch_latest_release
    download_package
    install_package
  fi

  if ! check_firecracker_installed; then
    fetch_firecracker_release
    download_and_extract_firecracker
  fi

  check_and_remove_nunet_data
  set_passphrase
  set_organization_and_identity_names
  setup_nunet_capabilities

  USER_DID=$(nunet key did "$USER_NAME")
  DMS_DID=$(nunet key did "$DMS_NAME")

  DEFAULT_EXPIRY=$(date -d "+30 days" +%Y-%m-%d)
  EXPIRY_DATE=${DEFAULT_EXPIRY}

  create_stricted_network
  grant_network_permission
  join_network
  run_nunet_in_background
  onboard_compute_resources
  get_peer_id
  deploy_ensemble

  cleanup

  echo "Setup complete."
}

main
