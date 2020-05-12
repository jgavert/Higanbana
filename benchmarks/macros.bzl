
def src_core_benchmark(target_name):
  native.cc_binary(
    name = "bench_core_" + target_name,
    srcs = ["core/bench_" + target_name + ".cpp"],
    deps = ["//core:core", "catch-benchmark-main"],
    copts = select({
      "@bazel_tools//src/conditions:windows": ["/std:c++latest", "/arch:AVX", "/permissive-", "/await"],
      "//conditions:default": ["-std=c++2a", "-msse4.2", "-m64", "-pthread"],
    }),
    defines = ["_ENABLE_EXTENDED_ALIGNED_STORAGE", "CATCH_CONFIG_ENABLE_BENCHMARKING"],
    linkopts = select({
      "@bazel_tools//src/conditions:windows": ["/subsystem:CONSOLE"],
      "//conditions:default": ["-pthread"],
    }),
  )  
