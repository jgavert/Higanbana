#!/bin/bash

./fbuild  app-x64linux-vulkan-release
mkdir -p workspace
rm -rf workspace/*
cp bin/x64Linux-Vulkan-Release/App/App workspace/.
cd workspace
cp -r ../appspace/* .
#ln -s ../appspace/* .



