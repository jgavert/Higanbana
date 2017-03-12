#!/bin/bash

./fbuild app-x64linux-null-profile
mkdir -p workspace
cp bin/x64Linux-Null-Profile/App/App workspace/.
cp -R appspace/* workspace/.


