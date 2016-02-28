#!/bin/bash

./fbuild app-x64linux-vulkan-debug
mkdir -p workspace
rm -rf workspace/*
cp bin/x64Linux-Vulkan-Debug/App/App workspace/.
cd workspace
ln -s ../appspace/* .



