cc_library(
    name = "catch-benchmark-main",
    srcs = ["main.cpp"],
    deps = ["//ext:Catch2"],
    defines = ["CATCH_CONFIG_ENABLE_BENCHMARKING"],
    linkstatic = True
)

cc_binary(
    name = "benchmarks",
    srcs = glob(["src/**/*.cpp"]),
    deps = ["//core:core", "//graphics:graphics", "catch-benchmark-main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64"],
    }),
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE", "CATCH_CONFIG_ENABLE_BENCHMARKING"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE"],
      "//conditions:default": [""],
    }),
)