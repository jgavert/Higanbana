#!/bin/python3.6
import os

config = """.WindowsSDKBasePath10 = 'C:/Program Files (x86)/Windows Kits/10'

#if __WINDOWS__
.FazEPath = 'CURRENT_DIRECTORY'
.FBuildCache = 'C:/temp/fazecache'
.VulkanSDKBasePath = 'C:/VulkanSDK/1.0.65.1'
#endif
#if __LINUX__
.FazEPath = 'CURRENT_DIRECTORY'
.FBuildCache = '/tmp/.fbuild.fazecache'
.VulkanSDKBasePath = '/usr/lib'
#endif"""

curDir = os.getcwd().replace("\\", "/")
print("current directory: " + curDir)

config = config.replace("CURRENT_DIRECTORY", curDir)

VSPath = """C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional"""

with open('config.bff', 'w') as out:
  out.write(""".VSBasePath = '""")
  if os.path.isdir('C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional'):
    VSPath = """C:/Program Files (x86)/Microsoft Visual Studio/2017/Professional"""
  else:
    VSPath = """C:/Program Files (x86)/Microsoft Visual Studio/2017/Community"""

  out.write(VSPath)
  out.write("""'""")
  out.write("\n")
  

  VSVersion = sorted(next(os.walk(VSPath+"""/VC/Tools/MSVC/"""))[1], reverse=True)[0]
  VSRedist = sorted(next(os.walk(VSPath+"""/VC/Redist/MSVC/"""))[1], reverse=True)[0]

  out.write(""".VSVersion  = '""")
  out.write(VSVersion)
  out.write("""'""")
  out.write("\n")

  out.write(""".VSRedist   = '""")
  out.write(VSRedist)
  out.write("""'""")
  out.write("\n")

  sdkVersionList = sorted(next(os.walk('C:/Program Files (x86)/Windows Kits/10/Include'))[1], reverse=True)
  ourSdk = sdkVersionList[0]

  out.write(""".WindowsSDKSubVersion = '""")
  out.write(ourSdk)
  out.write("""'""")
  out.write("\n")

  #print(ourSdk)
  #print(VSVersion[0])
  #print(VSRedist[0])
  
  out.write(config)


# .VSVersion = '14.12.25827'
# .VSRedist = '14.12.25810'
# .Root   = '$VSBasePath$/VC/Tools/MSVC/$VSVersion$/bin/HostX64/x64'
# .helperPath = '$VSBasePath$/VC/Redist/MSVC/$VSRedist$/x64/Microsoft.VC141.CRT'