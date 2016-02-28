#!/bin/bash

./fbuild app-x64linux-null-debug
mkdir -p workspace
cp bin/x64Linux-Null-Debug/App/App workspace/.
cp -R appspace/* workspace/.


