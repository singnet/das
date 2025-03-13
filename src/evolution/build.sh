#!/bin/bash

pyinstaller --onefile ./evolution/main.py
mkdir -p bin
mv ./dist/main ./dist/evolution
mv ./dist/evolution ./bin/evolution
rm -rf build/ __pycache__