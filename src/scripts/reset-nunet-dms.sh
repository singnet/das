#!/bin/bash

if [ -d "$HOME/.nunet" ]; then
  echo "$HOME/.nunet exists. Do you want to delete the configurations? (y/n)"
  read -r response
  if [[ "$response" == "y" || "$response" == "Y" ]]; then
    rm -rf "$HOME/.nunet"
    echo "Configurations deleted."
  else
    echo "Configurations not deleted."
  fi
fi

if command -v nunet-cli &> /dev/null; then
  echo "'nunet-cli' binary is installed. Do you want to uninstall it? (y/n)"
  read -r response
  if [[ "$response" == "y" || "$response" == "Y" ]]; then
    sudo apt-get remove --purge nunet-cli -y
    echo "'nunet-cli' uninstalled."
  else
    echo "'nunet-cli' not uninstalled."
  fi
else
  echo "'nunet-cli' binary is not installed."
fi
