#!/bin/python3.5
import os

config = """
.WindowsSDKBasePath10 = 'C:/Program Files (x86)/Windows Kits/10'
.WindowsSDKSubVersion = '10.0.16299.0'
#if __WINDOWS__
.FazEPath = 'CURRENT_DIRECTORY'
.FBuildCache = 'C:/temp/fazecache'
.VulkanSDKBasePath = 'C:/VulkanSDK/1.0.65.0'
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
  if os.path.isdir('C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional'):
    out.write(""".VSBasePath     = 'C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional'""")
  else:
    out.write(""".VSBasePath     = 'C:/Program Files (x86)/Microsoft Visual Studio/2017/Community'""")
  out.write("\n")
  out.write(config)
