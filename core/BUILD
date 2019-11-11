
cc_library(
  name = "core",
  srcs = glob(["src/**/*.cpp"]),
  hdrs = glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
  strip_include_prefix = "src",
  deps = ["//ext:GSL", "//ext:nlohmann_json"],
  copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
  defines = [],
  visibility = ["//visibility:public"], 
)