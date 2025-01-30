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

if command -v nunet &> /dev/null; then
  echo "'nunet' binary is installed. Do you want to uninstall it? (y/n)"
  read -r response
  if [[ "$response" == "y" || "$response" == "Y" ]]; then
    sudo apt-get remove --purge nunet-dms -y
    echo "'nunet-dms' uninstalled."
  else
    echo "'nunet' not uninstalled."
  fi
else
  echo "'nunet' binary is not installed."
fi
