#!/bin/python3.5
import os

config = """
.VSBasePath     = 'C:/Program Files (x86)/Microsoft Visual Studio/2017/Community'
.WindowsSDKBasePath10 = 'C:/Program Files (x86)/Windows Kits/10'
.WindowsSDKSubVersion = '10.0.15063.0'
#if __WINDOWS__
.FazEPath = 'CURRENT_DIRECTORY'
.FBuildCache = 'C:/temp/fazecache'
.VulkanSDKBasePath = 'C:/VulkanSDK/1.0.54.0'
#endif
#if __LINUX__
.FazEPath = 'CURRENT_DIRECTORY'
.FBuildCache = '/tmp/.fbuild.fazecache'
.VulkanSDKBasePath = '/usr/lib'
#endif"""

curDir = os.getcwd().replace("\\", "/")
print("current directory: " + curDir)

config = config.replace("CURRENT_DIRECTORY", curDir)

with open('config.bff', 'w') as out:
        out.write(config)
