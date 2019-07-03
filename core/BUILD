
cc_library(
  name = "core",
  srcs = glob(["src/**/*.cpp"]),
  hdrs = glob(["src/**/*.hpp"]) + glob(["src/**/*.h"]),
  strip_include_prefix = "src",
  deps = [],
  copts = select({
          "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
          "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
  visibility = ["//visibility:public"], 
)


# is no work on windows, googletest fails to load :)... not yet possible on windows the way I care to do it.
cc_test(
    name = "tests",
    srcs = glob(["tests/**/*.cc"]),
    deps = [
        "core",
        "@googletest//:gtest_main",
    ],
)