#!/bin/python3.6
import re
import os
import json

def natural_sort(l): 
  convert = lambda text: int(text) if text.isdigit() else text.lower() 
  alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ] 
  return sorted(l, key = alphanum_key)

# config = """
# mkdir "ext\\vulkan\\Lib"
# mkdir "ext\\vulkan\\Include"
# rmdir "ext\\vulkan"
# mkdir "ext\\vulkan"
# mklink /D "ext\\vulkan\\Lib" "C:\\VulkanSDK\\VULKAN_VERSION\\Lib"
# mklink /D "ext\\vulkan\\Include" "C:\\VulkanSDK\\VULKAN_VERSION\\Include"
# """
# 
# curDir = os.getcwd().replace("\\", "/")
# print("current directory: " + curDir)
# 
# config = config.replace("CURRENT_DIRECTORY", curDir)
# 
# VulkanVersion = natural_sort(next(os.walk("""C:/VulkanSDK/"""))[1])[-1]
# print("vulkan version: " + VulkanVersion)
# 
# config = config.replace("VULKAN_VERSION", VulkanVersion)

#with open('setup_vulkan.bat', 'w') as out:
#  out.write(config)

make_BUILDS = False
blob = True

# BUILD file generation, mostly experiment with blob or not.

def glob(dir, file_extension_filter = ""):
  walked = next(os.walk(dir))
  dirsToTraverse = []
  allFiles = []
  for dir in walked[1]:
    dirsToTraverse.append(walked[0] + "/" + dir)
  for file in walked[2]:
    if file.endswith(file_extension_filter):
      allFiles.append(walked[0] + "/" + file)
  while(len(dirsToTraverse) > 0):
    nextDir = dirsToTraverse.pop(0)
    walk = next(os.walk(nextDir))
    for dir in walk[1]:
      dirsToTraverse.append(nextDir + "/" + dir)
    for file in walk[2]:
      if file.endswith(file_extension_filter):
        allFiles.append(nextDir + "/" + file)
  return allFiles


def globForBazel(dir, file_extension):
  files = glob(dir, file_extension)
  cwd = dir + "/";
  cwdLen = len(cwd)
  files_t = list(map(lambda x: x[cwdLen:] if x[0:cwdLen] == cwd else x, files))
  return files_t

def build_template_library(name, srcs, hdrs, deps, copts):
  template = """
cc_library(
  name = "REPLACE_NAME",
  srcs = REPLACE_SRCS,
  hdrs = REPLACE_HDRS,
  strip_include_prefix = "src",
  deps = REPLACE_DEPS,
  copts = REPLACE_COPTS,
  visibility = ["//visibility:public"], 
)
"""
  edit = template.replace("REPLACE_NAME", name)
  edit = edit.replace("REPLACE_SRCS", srcs)
  edit = edit.replace("REPLACE_HDRS", hdrs)
  edit = edit.replace("REPLACE_DEPS", deps)
  edit = edit.replace("REPLACE_COPTS", copts)
  return edit

def make_library_BUILD(name, glob, deps, copts):
  srcs = """glob(["**/*.cpp"])"""
  hdrs = """glob(["**/*.hpp"])"""
  if glob:
    srcs = json.dumps(globForBazel(name, ".cpp"))
    hdrs = json.dumps(globForBazel(name, ".hpp") + globForBazel(name, ".h"))
  return build_template_library(name, srcs, hdrs, deps, copts)

if make_BUILDS:
  graphics_deps = """[
      "//core:core",
      "//ext:STB"] + select({
          "@bazel_tools//src/conditions:windows": 
              ["//ext:DirectXShaderCompiler",
               "//ext:WinPixEventRuntime", 
               "@VulkanSDKWin//:Vulkan",
               "@DX12//:DX12", 
               "@DXGI//:DXGI", 
               "@DXGUID//:DXGUID"],
          "//conditions:default": [],
    })"""

  copts = """select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    })"""
  coreBUILD = make_library_BUILD("core", blob, "[]", copts)
  graphicsBUILD = make_library_BUILD("graphics", blob, graphics_deps, copts)

  # print(coreBUILD)
  # print(graphicsBUILD)

  with open('higanbana/core/BUILD', 'w') as out:
    out.write(coreBUILD)

  with open('higanbana/graphics/BUILD', 'w') as out:
    out.write(graphicsBUILD)