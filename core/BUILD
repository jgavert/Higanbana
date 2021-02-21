
cc_library(
  name = "core_coro",
  srcs = glob(["src/**/*.cpp"]),
  hdrs = glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
  strip_include_prefix = "src",
  deps = ["//ext:GSL", "//ext:nlohmann_json", "//ext:higan_coroutines"] + select({
          "@bazel_tools//src/conditions:windows": 
              ["//ext:WinPixEventRuntime", "@DirectInput8//:DirectInput8", "@XInput//:XInput"],
          "//conditions:default": [],
  }),
  copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7", "-ftime-trace"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
  defines = ["JGPU_COROUTINES"],
  visibility = ["//visibility:public"], 
)

cc_library(
  name = "core",
  srcs = glob(["src/**/*.cpp"]),
  hdrs = glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
  strip_include_prefix = "src",
  deps = ["//ext:GSL", "//ext:nlohmann_json"] + select({
          "@bazel_tools//src/conditions:windows": 
              ["//ext:WinPixEventRuntime", "@DirectInput8//:DirectInput8", "@XInput//:XInput"],
          "//conditions:default": [],
  }),
  copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX2", "/permissive-", "/Z7", "-ftime-trace"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
  defines = [],
  visibility = ["//visibility:public"], 
)