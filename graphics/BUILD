cc_library(
  name = "graphics",
  srcs = glob(["**/*.cpp"]),
  hdrs = glob(["**/*.hpp"]),
  strip_include_prefix = "src",
  deps = [
      "//core:core",
      "//ext:IMGui",
      "//ext:DirectXShaderCompiler",
      "//ext:STB"] + select({
          "@bazel_tools//src/conditions:windows": 
              ["//ext:WinPixEventRuntime", 
               "@VulkanSDKWin//:Vulkan",
               "@DX12//:DX12", 
               "@DXGI//:DXGI", 
               "@DXGUID//:DXGUID"],
          "//conditions:default": [],
    }),
  copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
  visibility = ["//visibility:public"], 
)